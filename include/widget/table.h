#ifndef WIDGET_TABLE_H
#define WIDGET_TABLE_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

/**
 * @brief Sort order for table columns.
 */
typedef enum {
    SORT_NONE = 0,
    SORT_ASC,
    SORT_DESC
} TuiSortOrder;

/**
 * @brief A sortable, selectable table widget.
 */
typedef struct TuiTable {
    struct ncplane* plane;
    char** headers;
    char** cells;       /* row-major: cells[row * col_count + col] */
    int* col_widths;
    TuiSortOrder* col_sort;
    int row_count;
    int col_count;
    int row_capacity;
    int col_capacity;
    int selected_row;
    int scroll_off;
    int height;
    int width;
    bool focused;
    unsigned fg_header;
    unsigned fg_text;
    unsigned fg_selected;
    unsigned fg_separator;
    unsigned bg_normal;
    unsigned bg_header;
    unsigned bg_selected;
    unsigned bg_alt;
    int _type_id;
} TuiTable;

/**
 * @brief Create a table widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows (>= 2 for header + 1 row).
 * @param col_count Initial column count.
 * @return New table, or NULL on failure.
 */
TuiTable* tui_table_create(struct ncplane* parent, int y, int x,
                             int width, int height, int col_count);

/**
 * @brief Destroy the table.
 * @param table Table, or NULL.
 */
void tui_table_destroy(TuiTable* table);

/**
 * @brief Set a column header.
 * @param table Table.
 * @param col Column index.
 * @param header Header text. Copied internally.
 * @return true on success.
 */
bool tui_table_set_header(TuiTable* table, int col, const char* header);

/**
 * @brief Add a row of data.
 * @param table Table.
 * @param values Array of strings (must have col_count entries).
 * @return Row index on success, -1 on failure.
 */
int tui_table_add_row(TuiTable* table, const char** values);

/**
 * @brief Get the selected row.
 * @param table Table.
 * @return Selected row index, or -1.
 */
int tui_table_get_selected(const TuiTable* table);

/**
 * @brief Set the selected row.
 * @param table Table.
 * @param index Row index (clamped to valid range).
 */
void tui_table_set_selected(TuiTable* table, int index);

/**
 * @brief Handle a key event.
 * @param table Table.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_table_handle_key(TuiTable* table, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param table Table.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_table_handle_mouse(TuiTable* table, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the table.
 * @param table Table, or NULL.
 */
void tui_table_render(TuiTable* table);

/**
 * @brief Set focus state.
 * @param table Table.
 * @param focused Focus state.
 */
void tui_table_set_focused(TuiTable* table, bool focused);

/**
 * @brief Get the row count.
 * @param table Table.
 * @return Number of rows.
 */
int tui_table_get_row_count(const TuiTable* table);

/**
 * @brief Get a cell value.
 * @param table Table.
 * @param row Row index.
 * @param col Column index.
 * @return Cell text, or NULL.
 */
const char* tui_table_get_cell(const TuiTable* table, int row, int col);

#endif /* WIDGET_TABLE_H */
