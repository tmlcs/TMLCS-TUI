/**
 * @file tmlcs_tui.h
 * @brief TMLCS-TUI — Terminal User Interface Framework
 *
 * Umbrella header: include this single header to get the entire public API.
 *
 * @version 0.5.0
 * @copyright MIT License
 */
#ifndef TMLCS_TUI_H
#define TMLCS_TUI_H

/* Core types and theme */
#include "core/types.h"
#include "core/theme.h"

/* Core modules */
#include "core/logger.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme_loader.h"
#include "core/help.h"
#include "core/clipboard.h"
#include "core/widget.h"
#include "core/layout.h"
#include "core/loop.h"
#include "core/keymap.h"
#include "core/error.h"
#include "core/utf8.h"
#include "core/debug.h"
#include "core/anim.h"

/* Widgets */
#include "widget/text_input.h"
#include "widget/label.h"
#include "widget/button.h"
#include "widget/progress.h"
#include "widget/list.h"
#include "widget/textarea.h"
#include "widget/checkbox.h"
#include "widget/context_menu.h"
#include "widget/slider.h"
#include "widget/radio_group.h"
#include "widget/spinner.h"
#include "widget/tab_container.h"
#include "widget/dropdown.h"
#include "widget/dialog.h"
#include "widget/table.h"
#include "widget/tree.h"
#include "widget/file_picker.h"

#endif /* TMLCS_TUI_H */
