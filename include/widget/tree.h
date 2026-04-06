#ifndef WIDGET_TREE_H
#define WIDGET_TREE_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiTreeCb)(int node_index, void* userdata);

/**
 * @brief A tree node in the tree widget.
 */
typedef struct TuiTreeNode {
    char* label;
    int depth;
    int parent;       /* Index of parent node, -1 for root */
    bool expanded;
    bool is_leaf;
} TuiTreeNode;

/**
 * @brief A hierarchical tree widget with expand/collapse.
 */
typedef struct TuiTree {
    struct ncplane* plane;
    TuiTreeNode* nodes;
    int count;
    int capacity;
    int selected;
    int height;
    int width;
    int scroll_off;
    bool focused;
    TuiTreeCb cb;
    void* userdata;
    unsigned fg_expanded;
    unsigned fg_collapsed;
    unsigned fg_selected;
    unsigned fg_label;
    unsigned bg_normal;
    unsigned bg_selected;
    int _type_id;
} TuiTree;

/**
 * @brief Create a tree widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows.
 * @param cb Callback on node selection. May be NULL.
 * @param userdata Passed to callback.
 * @return New tree, or NULL on failure.
 */
TuiTree* tui_tree_create(struct ncplane* parent, int y, int x,
                          int width, int height,
                          TuiTreeCb cb, void* userdata);

/**
 * @brief Destroy the tree.
 * @param tree Tree, or NULL.
 */
void tui_tree_destroy(TuiTree* tree);

/**
 * @brief Add a root-level node.
 * @param tree Tree.
 * @param label Node label. Copied internally.
 * @param is_leaf true if the node has no children.
 * @return Node index on success, -1 on failure.
 */
int tui_tree_add_node(TuiTree* tree, const char* label, bool is_leaf);

/**
 * @brief Add a child node to an existing node.
 * @param tree Tree.
 * @param parent_idx Parent node index.
 * @param label Child label. Copied internally.
 * @param is_leaf true if the child has no children.
 * @return Node index on success, -1 on failure.
 */
int tui_tree_add_child(TuiTree* tree, int parent_idx, const char* label, bool is_leaf);

/**
 * @brief Toggle a node's expanded state.
 * @param tree Tree.
 * @param index Node index.
 */
void tui_tree_toggle(TuiTree* tree, int index);

/**
 * @brief Get the selected node.
 * @param tree Tree.
 * @return Selected node index, or -1.
 */
int tui_tree_get_selected(const TuiTree* tree);

/**
 * @brief Set the selected node.
 * @param tree Tree.
 * @param index Node index (clamped to valid visible range).
 */
void tui_tree_set_selected(TuiTree* tree, int index);

/**
 * @brief Handle a key event.
 * @param tree Tree.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_tree_handle_key(TuiTree* tree, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param tree Tree.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_tree_handle_mouse(TuiTree* tree, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the tree.
 * @param tree Tree, or NULL.
 */
void tui_tree_render(TuiTree* tree);

/**
 * @brief Set focus state.
 * @param tree Tree.
 * @param focused Focus state.
 */
void tui_tree_set_focused(TuiTree* tree, bool focused);

/**
 * @brief Get the node count.
 * @param tree Tree.
 * @return Total node count (including hidden).
 */
int tui_tree_get_count(const TuiTree* tree);

#endif /* WIDGET_TREE_H */
