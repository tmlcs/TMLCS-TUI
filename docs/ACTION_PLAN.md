# Plan de Acción de Calidad — TMLCS-TUI

**Objetivo:** Elevar la calidad del código de 2.4/5 a ≥3.5/5 en 6-8 semanas  
**Metodología:** Fases iterativas (P0 → P1 → P2 → P3)  
**Métricas de éxito:** Cero crashes en testing manual + 80% cobertura de código + 0 memory leaks (Valgrind)

---

## Fase 0: Estabilización (Semana 1)

### Objetivo
Eliminar crashes críticos y problemas de memory safety que impidan desarrollo estable.

### Tareas

#### P0-1: Arreglar dangling pointer `g_active_input` (15 min)
**Responsable:** Dev  
**Criterios de éxito:**
- [ ] `g_active_input` se limpia en `console_on_destroy()`
- [ ] Event loop verifica `g_active_input != NULL` antes de llamar `tui_text_input_handle_key()`
- [ ] Testing: Cerrar consola y seguir usando app → no SEGFAULT

**Cambios:**
```c
// main.c, línea ~80
if (key >= NCKEY_BUTTON1 && key <= NCKEY_BUTTON11) {
    tui_manager_process_mouse(mgr, key, &ni);
} else if (key == NCKEY_MOTION) {
    tui_manager_process_mouse(mgr, key, &ni);
} else {
    bool consumed = false;
    if (g_active_input && g_active_input->focused) {
        consumed = tui_text_input_handle_key(g_active_input, key, &ni);
    }
    if (!consumed) {
        tui_manager_process_keyboard(mgr, key, &ni);
    }
}

// console_on_destroy:
void console_on_destroy(TuiWindow* win) {
    if (win->text_input) {
        tui_text_input_destroy((TuiTextInput*)win->text_input);
        win->text_input = NULL;
    }
    if (g_active_input == (TuiTextInput*)win->text_input) {
        g_active_input = NULL;
    }
}
```

#### P0-2: Null-terminator en `strncpy` (15 min)
**Responsable:** Dev  
**Criterios:**
- [ ] `tab.c: tui_tab_create()` → `tab->name[sizeof(tab->name)-1] = '\0';`
- [ ] `workspace.c: tui_workspace_create()` → `ws->name[sizeof(ws->name)-1] = '\0';`
- [ ] Testing: Crear tab con nombre de 100 caracteres → no overflow

#### P0-3: Validar todas las asignaciones de memoria (45 min)
**Responsable:** Dev  
**Criterios:**
- [ ] Agregar `if (!ptr) { tui_log(LOG_ERROR, "OOM en %s", __func__); return NULL; }` en:
  - `tui_tab_create()`
  - `tui_window_create()`
  - `tui_workspace_create()`
  - `tui_manager_create()`
  - `tui_text_input_create()`
- [ ] `ncplane_create()` → verificar retorno, limpiar y retornar NULL si falla
- [ ] Testing: Simular OOM (no factible fácilmente) pero verificar que no se bloquee en allocs

#### P0-4: Limpiar `dragged_window`/`resizing_window` al destruir workspace (30 min)
**Responsable:** Dev  
**Criterios:**
- [ ] En `tui_manager_remove_active_workspace()`, antes de destruir workspace:
  ```c
  TuiWorkspace* ws = mgr->workspaces[idx];
  // Limpiar referencias a ventanas de este workspace
  if (mgr->dragged_window && window_belongs_to_workspace(mgr->dragged_window, ws))
      mgr->dragged_window = NULL;
  if (mgr->resizing_window && window_belongs_to_workspace(mgr->resizing_window, ws))
      mgr->resizing_window = NULL;
  ```
- [ ] Implementar helper `window_belongs_to_workspace(TuiWindow*, TuiWorkspace*)` (recorrer tabs)
- [ ] Testing: Cerrar workspace mientras se arrastra/resize una ventana → no crash

---

## Fase 1: Refactorización y Separación (Semana 2-3)

### Objetivo
Separar el core framework del demo, establishing a clean library API.

### Tareas

#### P1-5: Separar core framework del demo (2-3 hrs)
**Responsable:** Dev + Reviewer  
**Criterios de éxito:**
- [ ] Mover `log_server_render`, `console_render`, `my_window_render` a `examples/demo.c`
- [ ] `include/core/*.h` y `src/core/*.c` no dependen de `main.c`
- [ ] Crear `Makefile` target `libtmlcs_tui.a` (static library)
- [ ] Demo se enlaza contra la librería y funciona igual
- [ ] CMakeLists.txt opcional (pero no requerido)

**Estructura nueva:**
```
tmlcs-tui/
├── include/
│   └── tmlcs_tui.h    # Re-export all core headers (convenience)
├── src/
│   ├── core/          # Librería pura (sin main)
│   └── widget/
├── examples/
│   └── demo.c         # Demo original movido aquí
├── tests/              # Nueva carpeta
├── Makefile
└── README.md
```

#### P1-6: Sistema de constantes/themes (1 hr)
**Responsable:** Dev  
**Criterios:**
- [ ] Crear `src/core/theme.h`:
  ```c
  #define THEME_FG_NORMAL 0xf8f8f2
  #define THEME_BG_NORMAL 0x21222c
  // ...
  #define TAB_BAR_HEIGHT 1
  #define TITLE_BAR_HEIGHT 1
  #define WINDOW_MIN_HEIGHT 4
  #define WINDOW_MIN_WIDTH 15
  #define TAB_BAR_START_X 30
  ```
- [ ] Refactorizar todos los archivos `core/` y `widget/` para usar las constantes
- [ ] NO modificar `main.c` en esta fase (se hará en Fase 2)

#### P1-7: Logging de errores críticos (30 min)
**Responsable:** Dev  
**Criterios:**
- [ ] En cada `if (!ptr) return NULL;` agregar `tui_log(LOG_ERROR, "OOM en %s", __func__);`
- [ ] En `text_input.c` cuando `ncplane_create` falle
- [ ] Verificar que logger esté inicializado antes de loguear (podría no estarlo en `tui_manager_create` → postergar logging hasta init)

---

## Fase 2: Testing y CI (Semana 4)

### Objetivo
Establecer una baseline de calidad con tests automatizados y continuous integration.

### Tareas

#### P2-8: Framework de testing (8-12 hrs)
**Responsable:** Dev + QA  
**Criterios de éxito:**
- [ ] Agregar submodule o librería Unity (https://github.com/ThrowTheSwitch/Unity)
- [ ] Estructura de tests:
  ```
  tests/
  ├── unity/
  ├── test_logger.c
  ├── test_text_input.c
  ├── test_tab.c
  └── test_workspace.c
  ```
- [ ] Tests unitarios mínimos:
  - `logger`: funcionamiento ring buffer, thread-safety básico
  - `text_input`: inserción, borrado, scroll
  - `tab`: add/remove windows, active index
  - `workspace`: add/remove tabs, set active
- [ ] Mocking de `notcurses` mediante stubs (solo para estructuras de datos)
- [ ] `make test` compila y ejecuta tests con 100% éxito

#### P2-9: CI/CD con GitHub Actions (4-6 hrs)
**Responsable:** DevOps  
**Criterios:**
- [ ] `.github/workflows/ci.yml` que:
  - Instala notcurses (apt install libnotcurses-dev)
  - Ejecuta `make all`
  - Ejecuta `make test`
  - Ejecuta `valgrind --leak-check=full ./examples/demo` (o `make valgrind`)
- [ ] Badge en README: `build | passing`
- [ ] PRs bloquean merge si falla CI

#### P2-10: Dirty regions en render (6-8 hrs)
**Responsable:** Dev + Performance engineer  
**Criterios:**
- [ ] Añadir flag `bool dirty` a `TuiWindow`
- [ ] `tui_window_mark_dirty(win)` y `tui_window_clear_dirty(win)`
- [ ] En `tui_manager_render()`:
  ```c
  for (int i = 0; i < tab->window_count; i++) {
      TuiWindow* win = tab->windows[i];
      if (win->dirty) {
          if (win->render_cb) win->render_cb(win);
          win->dirty = false;
      }
  }
  ```
- [ ] Mark dirty cuando:
  - Ventana movida (drag finish)
  - Ventana redimensionada
  - TextInput modifica contenido
  - Focus change
- [ ] Benchmark: Comparar CPU usage antes/después (usar `time` o `perf`)

#### P2-11: Refactor `tui_manager_process_mouse` (3-4 hrs)
**Responsable:** Dev  
**Criterios:**
- [ ] Extraer funciones auxiliares:
  - `static bool _handle_tab_click(TuiManager*, const struct ncinput*)`
  - `static bool _handle_window_click(TuiManager*, TuiWindow*, const struct ncinput*, int wly, int wlx)`
  - `static void _start_drag(TuiManager*, TuiWindow*, int wly, int wlx)`
  - `static void _start_resize(TuiManager*, TuiWindow*, const struct ncinput*)`
  - `static void _process_drag(TuiManager*, const struct ncinput*)`
  - `static void _process_resize(TuiManager*, const struct ncinput*)`
- [ ] Mantener misma semántica, reducir complejidad ciclomática a <10 por función
- [ ] Tests unitarios para cada helper (usando mock inputs)

---

## Fase 3: Documentación y Release (Semana 5-6)

### Objetivos
Generar documentación completa y preparar primer release estable v0.1.0.

### Tareas

#### P3-12: Sistema de temas configurables (4 hrs)
**Responsable:** Dev  
**Criterios:**
- [ ] `src/core/theme.c` que carga colores desde `~/.config/tmlcs-tui/theme.toml` (fallback a defaults)
- [ ] Estructura TOML:
  ```toml
  [colors]
  fg_normal = 0xf8f8f2
  bg_normal = 0x21222c
  # ...
  [layout]
  tab_bar_start_x = 30
  window_min_height = 4
  window_min_width = 15
  ```
- [ ] Función `tui_theme_load(const char* path)` que sobreescribe constantes en `theme.h`
- [ ] Documentar formato de tema en README

#### P3-13: Doxygen API docs (2 hrs)
**Responsable:** Doc writer  
**Criterios:**
- [ ] Instalar Doxygen
- [ ] Agregar comentarios `/** */` a todas las funciones públicas en headers
- [ ] Generar `docs/html/` y subir a GitHub Pages
- [ ] Badge en README: `docs | stable`

#### P3-14: README completo (1 hr)
**Responsable:** Doc writer  
**Contenido:**
- [ ] Descripción del proyecto (1 párrafo)
- [ ] Captura de pantalla (screenshot.png)
- [ ] Características principales
- [ ] Instalación de dependencias (notcurses)
- [ ] Build & run: `make && ./mi_tui`
- [ ] API Quickstart (ejemplo mínimo)
- [ ] Lista de keybindings
- [ ] Sección "Contributing" con estándares de código (clang-format)

#### P3-15: Release v0.1.0 (1 hr)
**Responsable:** Release manager  
**Criterios:**
- [ ] Git tag `v0.1.0` con mensaje semántico
- [ ] GitHub Release con:
  - Binario estático compilado (x86_64-linux)
  - Changelog generado desde commits desde última versión
  - Instrucciones de instalación
- [ ] Actualizar `AUDIT.md` con métricas post-refactor

---

## Fase 4: Sprints de Mejora Continua (Semana 7-8+)

### Objetivo
Implementar mejoras UX y advanced features.

#### P2 (continuación): Soporte scroll wheel (2 hrs)
**Tarea:** En `tui_manager_process_mouse`, manejar `NCKEY_SCROLL_UP/DOWN` para:
- Scroll en ventanas de log si el contenido excede altura
- Scroll en TextInput (si hay texto fuera de vista)

#### P3 (continuación): Right-click context menus (6-8 hrs)
- Menú emergente con opciones: Close, Rename, Move, Resize
- Usar `ncplane` para crear ventana modal
- Callbacks para cada acción

---

## Métricas de Seguimiento

| Métrica | Línea base | Objetivo (Sem 6) | Herramienta |
|---------|------------|------------------|-------------|
| Cobertura de código | 0% | ≥80% | gcov/lcov |
| Memory leaks | Desconocido | 0 leaks (valgrind) | Valgrind |
| Complejidad ciclomática avg | ~12 | ≤8 | lizard, codeclimate |
| Tiempo de compilación | ~2s | ≤2s | time make |
| CPU idle (sin input) | ~2% | ≤2% | top, perf |
| Bugs reportados (GitHub Issues) | 4 críticos | 0 críticos | GitHub Issues |
| Tiempo de setup para contributor | >30 min | ≤10 min | Documentación |

---

## Checklist de Pre-Release (v0.1.0)

- [ ] **Code Quality**
  - [ ] No memory leaks (valgrind --leak-check=full)
  - [ ] No use-after-free (ASAN)
  - [ ] Código formateado con clang-format
  - [ ] Zero warnings de compilación con `-Wall -Wextra`

- [ ] **Testing**
  - [ ] ≥80% line coverage
  - [ ] Tests unitarios pasan en CI
  - [ ] Tests de integración (demo runs 60s sin crashes)

- [ ] **Documentation**
  - [ ] API docs generadas y disponibles
  - [ ] README con instalación y uso
  - [ ] CHANGELOG.md actualizado
  - [ ] Captura de pantalla en README

- [ ] **CI/CD**
  - [ ] Build passing en Linux (Ubuntu latest)
  - [ ] Tests passing
  - [ ] Valgrind passing
  - [ ] Code coverage uploaded (Codecov)

- [ ] **Release**
  - [ ] Git tag `v0.1.0`
  - [ ] GitHub Release con assets
  - [ ] Binario estático para linux amd64

---

## Riesgos y Mitigaciones

| Riesgo | Prob. | Impacto | Mitigación |
|--------|-------|---------|------------|
| Notcurses API cambia (breaking changes) | Media | Alto | Pin a versión específica en README (ej: notcurses 3.0.10) |
| Regresiones al refactorar mouse handler | Alta | Medio | Test unitarios exhaustive + manual testing checklist |
| OOM en sistemas embebidos no detectado | Baja | Alto | Simular OOM con malloc hook en tests |
| Dificultad para mockear notcurses en tests | Media | Medio | Crear wrapper API `nc_*` que pueda ser stubeado |

---

## Asignación de Recursos

- **Desarrollador principal:** Todas las tareas P0-P1
- **QA/Testing:** P2-8, P2-9, P2-10, P2-11
- **DevOps:** P2-9 (CI/CD)
- **Tech Writer:** P3-13, P3-14
- **Release Manager:** P3-15

*Nota: En proyecto individual, mismas personas cubren múltiples roles.*

---

## Cronograma Gantt (6 semanas)

```
Semana 1: [████████░░] P0-1..4 (Estabilización)
Semana 2: [████████░░] P1-5..7 (Separación + Themes)
Semana 3: [████████░░] P1-5..7 (continuación) + P2-8..9 (Testing/CI)
Semana 4: [░░░░░░░░░░] P2-10..11 (Dirty regions + Refactor mouse)
Semana 5: [░░░░░░░░░░] P3-12..14 (Temas, Docs, README)
Semana 6: [░░░░░░░░░░] P3-15 (Release v0.1.0) + Buffer para retrasos
```

---

*Fin del plan de acción detallado.*
