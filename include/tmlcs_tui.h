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

/* Widgets */
#include "widget/text_input.h"
#include "widget/label.h"
#include "widget/button.h"
#include "widget/progress.h"
#include "widget/list.h"
#include "widget/textarea.h"
#include "widget/checkbox.h"
#include "widget/context_menu.h"

#endif /* TMLCS_TUI_H */
