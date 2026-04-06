#include "widget/tree.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>
#include <string.h>

void tui_tree_ensure_registered(void);
int tui_tree_get_type_id(void);

static int s_tree_type_id = -1;

static void ensure_capacity(TuiTree* tree) {
    if (tree->count >= tree->capacity) {
        int new_cap = tree->capacity * 2;
        TuiTreeNode* new_nodes = (TuiTreeNode*)realloc(tree->nodes, (size_t)new_cap * sizeof(TuiTreeNode));
        if (!new_nodes) return;
        tree->nodes = new_nodes;
        tree->capacity = new_cap;
    }
}

TuiTree* tui_tree_create(struct ncplane* parent, int y, int x,
                          int width, int height,
                          TuiTreeCb cb, void* userdata) {
    if (!parent || width < 4 || height < 1) return NULL;
    TuiTree* tree = (TuiTree*)calloc(1, sizeof(TuiTree));
    if (!tree) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    tree->plane = ncplane_create(parent, &opts);
    if (!tree->plane) { free(tree); return NULL; }

    tree->capacity = 16;
    tree->nodes = (TuiTreeNode*)calloc((size_t)tree->capacity, sizeof(TuiTreeNode));
    if (!tree->nodes) { ncplane_destroy(tree->plane); free(tree); return NULL; }
    tree->count = 0;
    tree->selected = -1;
    tree->height = height;
    tree->width = width;
    tree->scroll_off = 0;
    tree->focused = false;
    tree->cb = cb;
    tree->userdata = userdata;
    tree->fg_expanded = THEME_FG_TAB_ACTIVE;
    tree->fg_collapsed = THEME_FG_TAB_INA;
    tree->fg_selected = THEME_FG_TAB_ACTIVE;
    tree->fg_label = THEME_FG_DEFAULT;
    tree->bg_normal = THEME_BG_DARKEST;
    tree->bg_selected = THEME_BG_TAB_ACTIVE;

    tui_tree_ensure_registered();
    tree->_type_id = s_tree_type_id;

    tui_tree_render(tree);
    return tree;
}

void tui_tree_destroy(TuiTree* tree) {
    if (!tree) return;
    for (int i = 0; i < tree->count; i++) free(tree->nodes[i].label);
    free(tree->nodes);
    if (tree->plane) ncplane_destroy(tree->plane);
    free(tree);
}

int tui_tree_add_node(TuiTree* tree, const char* label, bool is_leaf) {
    if (!tree || !label) return -1;
    ensure_capacity(tree);
    if (tree->count >= tree->capacity) return -1;

    int idx = tree->count;
    tree->nodes[idx].label = strdup(label);
    tree->nodes[idx].depth = 0;
    tree->nodes[idx].parent = -1;
    tree->nodes[idx].expanded = !is_leaf;
    tree->nodes[idx].is_leaf = is_leaf;
    tree->count++;

    if (tree->selected == -1) tree->selected = 0;
    tui_tree_render(tree);
    return idx;
}

int tui_tree_add_child(TuiTree* tree, int parent_idx, const char* label, bool is_leaf) {
    if (!tree || !label || parent_idx < 0 || parent_idx >= tree->count) return -1;
    ensure_capacity(tree);
    if (tree->count >= tree->capacity) return -1;

    int parent_depth = tree->nodes[parent_idx].depth;

    /* Find insertion point: after all existing children of this parent */
    int insert_pos = parent_idx + 1;
    while (insert_pos < tree->count && tree->nodes[insert_pos].depth > parent_depth) {
        insert_pos++;
    }

    /* Shift nodes to make room */
    for (int i = tree->count; i > insert_pos; i--) {
        tree->nodes[i] = tree->nodes[i - 1];
        /* Fix parent indices for shifted nodes */
        if (tree->nodes[i].parent >= insert_pos) {
            tree->nodes[i].parent++;
        }
    }
    if (tree->selected >= insert_pos) tree->selected++;

    /* Insert new child */
    tree->nodes[insert_pos].label = strdup(label);
    tree->nodes[insert_pos].depth = parent_depth + 1;
    tree->nodes[insert_pos].parent = parent_idx;
    tree->nodes[insert_pos].expanded = !is_leaf;
    tree->nodes[insert_pos].is_leaf = is_leaf;
    tree->count++;

    /* Make parent not a leaf */
    tree->nodes[parent_idx].is_leaf = false;

    /* Make sure parent is expanded */
    tree->nodes[parent_idx].expanded = true;

    tui_tree_render(tree);
    return insert_pos;
}

void tui_tree_toggle(TuiTree* tree, int index) {
    if (!tree || index < 0 || index >= tree->count) return;
    if (tree->nodes[index].is_leaf) return;
    tree->nodes[index].expanded = !tree->nodes[index].expanded;
    tui_tree_render(tree);
}

int tui_tree_get_selected(const TuiTree* tree) {
    return tree ? tree->selected : -1;
}

void tui_tree_set_selected(TuiTree* tree, int index) {
    if (!tree) return;
    if (index < 0) index = 0;
    if (index >= tree->count) index = tree->count > 0 ? tree->count - 1 : -1;
    tree->selected = index;

    /* Adjust scroll */
    if (tree->selected < tree->scroll_off) {
        tree->scroll_off = tree->selected;
    } else if (tree->selected >= tree->scroll_off + tree->height) {
        tree->scroll_off = tree->selected - tree->height + 1;
    }
    tui_tree_render(tree);
}

void tui_tree_set_focused(TuiTree* tree, bool focused) {
    if (!tree) return;
    tree->focused = focused;
    tui_tree_render(tree);
}

int tui_tree_get_count(const TuiTree* tree) {
    return tree ? tree->count : 0;
}

/* Build a list of visible node indices */
static int build_visible_indices(const TuiTree* tree, int* out_indices, int max_out) {
    if (!tree || !out_indices || max_out <= 0) return 0;

    int count = 0;
    for (int i = 0; i < tree->count && count < max_out; i++) {
        /* Check if this node should be visible */
        bool visible = true;
        int check = tree->nodes[i].parent;
        while (check >= 0) {
            if (!tree->nodes[check].expanded) {
                visible = false;
                break;
            }
            check = tree->nodes[check].parent;
        }
        if (visible) {
            out_indices[count] = i;
            count++;
        }
    }
    return count;
}

bool tui_tree_handle_key(TuiTree* tree, uint32_t key, const struct ncinput* ni) {
    if (!tree || !ni || tree->count == 0) return false;

    int vis[128];
    int vis_count = build_visible_indices(tree, vis, 128);
    if (vis_count == 0) return false;

    /* Find current selection position in visible list */
    int vis_idx = -1;
    for (int i = 0; i < vis_count; i++) {
        if (vis[i] == tree->selected) {
            vis_idx = i;
            break;
        }
    }

    if (key == NCKEY_UP) {
        if (vis_idx > 0) {
            tui_tree_set_selected(tree, vis[vis_idx - 1]);
        }
        return true;
    }
    if (key == NCKEY_DOWN) {
        if (vis_idx < vis_count - 1) {
            tui_tree_set_selected(tree, vis[vis_idx + 1]);
        }
        return true;
    }
    if (key == NCKEY_LEFT) {
        /* Collapse node */
        if (tree->selected >= 0 && tree->selected < tree->count && !tree->nodes[tree->selected].is_leaf) {
            tree->nodes[tree->selected].expanded = false;
            tui_tree_render(tree);
        }
        return true;
    }
    if (key == NCKEY_RIGHT) {
        /* Expand node */
        if (tree->selected >= 0 && tree->selected < tree->count && !tree->nodes[tree->selected].is_leaf) {
            tree->nodes[tree->selected].expanded = true;
            tui_tree_render(tree);
        }
        return true;
    }
    if (key == ' ') {
        /* Toggle expand/collapse */
        if (tree->selected >= 0 && tree->selected < tree->count) {
            tui_tree_toggle(tree, tree->selected);
            if (tree->cb) tree->cb(tree->selected, tree->userdata);
        }
        return true;
    }
    return false;
}

bool tui_tree_handle_mouse(TuiTree* tree, uint32_t key, const struct ncinput* ni) {
    if (!tree || !ni || !tree->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int vis[128];
    int vis_count = build_visible_indices(tree, vis, 128);
    if (vis_count == 0) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(tree->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(tree->plane, &rows, &cols);
    (void)cols;

    int local_y = ni->y - plane_abs_y;
    if (local_y < 0 || local_y >= (int)rows) return false;

    int clicked_vis = tree->scroll_off + local_y;
    if (clicked_vis >= 0 && clicked_vis < vis_count) {
        int node_idx = vis[clicked_vis];
        tree->selected = node_idx;
        /* Toggle expand/collapse on click */
        if (!tree->nodes[node_idx].is_leaf) {
            tui_tree_toggle(tree, node_idx);
        }
        if (tree->cb) tree->cb(node_idx, tree->userdata);
        return true;
    }
    return false;
}

void tui_tree_render(TuiTree* tree) {
    if (!tree || !tree->plane) return;
    unsigned cols;
    ncplane_dim_yx(tree->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, tree->fg_label);
    ncchannels_set_bg_rgb(&base, tree->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(tree->plane, " ", 0, base);
    ncplane_erase(tree->plane);

    int vis[128];
    int vis_count = build_visible_indices(tree, vis, 128);

    int visible_rows = tree->height;
    int start = tree->scroll_off;
    if (start < 0) start = 0;

    for (int row = 0; row < visible_rows && (start + row) < vis_count; row++) {
        int node_idx = vis[start + row];
        if (node_idx < 0 || node_idx >= tree->count) continue;

        TuiTreeNode* node = &tree->nodes[node_idx];
        bool is_selected = (node_idx == tree->selected);

        int indent = node->depth * 2;
        if (indent >= (int)cols - 3) indent = (int)cols - 3;
        if (indent < 0) indent = 0;

        /* Draw indentation */
        for (int i = 0; i < indent; i++) {
            ncplane_set_fg_rgb(tree->plane, tree->fg_label);
            ncplane_putstr_yx(tree->plane, row, i, " ");
        }

        /* Draw expand/collapse icon */
        if (!node->is_leaf) {
            ncplane_set_fg_rgb(tree->plane, node->expanded ? tree->fg_expanded : tree->fg_collapsed);
            ncplane_putstr_yx(tree->plane, row, indent, node->expanded ? "\xe2\x96\xbc" : "\xe2\x96\xb6"); /* ▼ or ▶ */
        } else {
            ncplane_set_fg_rgb(tree->plane, tree->fg_label);
            ncplane_putstr_yx(tree->plane, row, indent, "  ");
        }

        /* Draw label */
        if (is_selected) {
            ncplane_set_fg_rgb(tree->plane, tree->fg_selected);
            ncplane_set_bg_rgb(tree->plane, tree->bg_selected);
        } else {
            ncplane_set_fg_rgb(tree->plane, tree->fg_label);
            ncplane_set_bg_rgb(tree->plane, tree->bg_normal);
        }
        int text_x = indent + 2;
        int max_text = (int)cols - text_x;
        if (max_text > 0) {
            int len = (int)strlen(node->label);
            int copy = len < max_text ? len : max_text;
            char buf[256];
            memcpy(buf, node->label, (size_t)copy);
            buf[copy] = '\0';
            ncplane_putstr_yx(tree->plane, row, text_x, buf);
        }
    }
}

/* === VTable === */

static bool v_tree_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_tree_handle_key((TuiTree*)widget, key, ni);
}

static bool v_tree_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_tree_handle_mouse((TuiTree*)widget, key, ni);
}

static void v_tree_render(void* widget) {
    tui_tree_render((TuiTree*)widget);
}

static bool v_tree_is_focusable(void* widget) { (void)widget; return true; }

static void v_tree_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = 5;
    if (out_w) *out_w = 20;
}

static const TuiWidgetIface s_tree_iface = {
    .handle_key = v_tree_handle_key,
    .handle_mouse = v_tree_handle_mouse,
    .render = v_tree_render,
    .is_focusable = v_tree_is_focusable,
    .preferred_size = v_tree_preferred_size,
};

void tui_tree_ensure_registered(void) {
    if (s_tree_type_id < 0) {
        s_tree_type_id = tui_widget_register(&s_tree_iface);
    }
}

int tui_tree_get_type_id(void) {
    tui_tree_ensure_registered();
    return s_tree_type_id;
}
