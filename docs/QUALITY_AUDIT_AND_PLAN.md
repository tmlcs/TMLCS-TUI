# 🔍 TMLCS-TUI — Auditoría de Calidad y Plan de Mejora Detallado

*Generado: 2026-04-05 — Basado en análisis de código*

---

## 📊 Estado Actual de Calidad (Scorecard)

| Categoría | Score | Estado | Comentarios |
|-----------|-------|--------|-------------|
| **Build System** | 9/10 | ✅ Excelente | -Wall -Wextra -Werror, make limpio |
| **Tests** | 7/10 | ✅ Bueno | 3 tests unitarios (logger) pasando |
| **Memory Safety** | 8/10 | ✅ Bueno | NULL checks en malloc/calloc, sin leaks obvios |
| **Buffer Safety** | 9/10 | ✅ Excelente | strncpy + snprintf, sin sprintf/strcpy |
| **Code Cleanliness** | 8/10 | ✅ Bueno | Código limpio, bien comentado |
| **Modularity** | 8/10 | ✅ Bueno | Separación clara: core/, widget/ |
| **Error Handling** | 7/10 | 🟡 Medio | Logs presentes pero sin niveles configurables en runtime |
| **API Design** | 7/10 | 🟡 Medio | Tipos opacos podrían mejorar encapsulación |
| **Documentation** | 4/10 | ❌ Deficiente | Solo comentarios en código, sin Doxygen/README |
| **CI/CD** | 2/10 | ❌ Ausente | Sin GitHub Actions ni pipeline |

**Score Global: 7.0/10** — *Código de buena calidad, listo para producción con mejoras menores*

---

## ✅ Fortalezas Identificadas

1. **Build impecable**: `-Wall -Wextra -Werror` sin warnings
2. **Seguridad de memoria**: Todos los `malloc`/`calloc` validados con NULL check
3. **Buffer safety**: Uso correcto de `strncpy()` y `snprintf()`, **sin `sprintf` peligrosos**
4. **Architecture limpia**:
   - `src/core/` → `logger.c`, `tab.c`, `workspace.c`, `window.c`, `manager/` (4 archivos)
   - `src/widget/` → `text_input.c`
   - `include/` headers bien organizados
5. **Tests unitarios**: 3 tests pasando (logger buffer wrap, basic ops, thread safety)
6. **Theme centralizado**: `include/core/theme.h` con constantes Dracula
7. **Manager implementado**: `src/core/manager/` con `state.c`, `render.c`, `input.c`, `utils.c` ✅
8. **tab.c ya limpio**: 108 LOC, sin código comentado muerto
9. **Notcurses bien integrado**: Render, mouse, eventos funcionales

---

## ⚠️ Issues Detectados (Ordenados por Prioridad)

### 🔴 Alta Prioridad

#### ISSUE-01: Falta .gitignore
**Archivo**: Raíz del proyecto
**Problema**: archivos `build/`, `mi_tui`, `*.o` no ignorados
**Fix**: Crear `.gitignore` estándar

#### ISSUE-02: Constantes mágicas residuales en manager/input.c
**Líneas 12, 47, 126, 170+**: Hardcoded `30` (offset_x tab bar), `4` (min width), `2` (padding)
**Problema**: Deberían ir a `theme.h`
**Fix**: Añadir `TAB_BAR_START_X` ya existe, usarlo consistentemente; crear `WINDOW_MIN_*` si no existen

#### ISSUE-03: Magic number `-10000` para off-screen
**Ubicaciones**: `workspace.c:18`, `workspace.c:101`, `tab.c:22`, `manager/state.c:36`
**Problema**: "Off-screen parking" hardcodeado, sin constante Named
**Fix**: Añadir `OFFSCREEN_Y = -10000` en `theme.h`

#### ISSUE-04: Validación de `realloc` en `tui_manager_add_workspace`
**Archivo**: `src/core/manager/state.c:47-49`
**Problema**: Si `realloc` falla, se hace `return` pero el workspace no se agrega — correcto, **pero** ≠ se pierde la nueva capacidad. Esto puede causar OOM más adelante sin recuperación.
**Fix**: Considerar manejo de error más robusto o abort controlado.

#### ISSUE-05: No hay test coverage para módulos core (tab, workspace, window, manager)
**Problema**: Solo `test_logger.c` existe, cobertura < 5%
**Fix**: Añadir tests unitarios usando Unity mockeando notcurses

### 🟡 Media Prioridad

#### ISSUE-06: API no-opaca (acceso directo a struct fields)
**Ubicación**: Varias funciones acceden a `tab->window_count`, `ws->tab_count`, etc.
**Problema**: Violación de encapsulación, dificulta cambios internos
**Fix**: Convertir `TuiTab`, `TuiWorkspace`, `TuiWindow` a *opaque types* en headers públicos

#### ISSUE-07: Falta logging en production builds
**Problema**: `tui_logger_init` se llama en `main.c`, pero siempre escribe a `/tmp/tmlcs_tui_debug.log` sin forma de desactivarlo
**Fix**: Añadir `tui_logger_set_level()` y `tui_logger_set_stdout()`

#### ISSUE-08: Text input no maneja multi-byte UTF-8
**Ubicación**: `src/widget/text_input.c:117-125`
**Problema**: Solo acepta ASCII 0x20-0x7E, ignora caracteres Unicode
**Fix**: Usar `nccell` o al menos permitir bytes > 0x7F en buffer (no interpretar)

#### ISSUE-09: Hardcoded color values en `manager/render.c`
**Ubicación**: Líneas 8, 12, 26, 35, 66, 79, 96, etc.
**Problema**: Aunque theme.h colores, hay duplicados literales (0x111116, 0x333344, etc.)
**Fix**: Reemplazar todos por constantes de `theme.h`

#### ISSUE-10: Faltan getters/setters para struct fields
**Problema**: Código cliente no puede acceder a `tab->name`, `win->id` sin exponer struct
**Fix**: Añadir `tui_tab_get_name()`, `tui_window_get_id()` si se necesita API pública

### 🟢 Baja Prioridad (Mejoras)

#### ISSUE-11: No hay MAKE install target
**Fix**: Añadir `install:` que copie headers a `/usr/local/include/tmlcs-tui/` y lib si aplica

#### ISSUE-12: No hay configuración `--enable-sanitizers` opcional
**Fix**: Añadir `CFLAGS_SAN = -fsanitize=address,undefined` y `make asan`

#### ISSUE-13: Falta documentación de API (Doxygen)
**Fix**: Añadir comentarios `/** */` en headers públicos y generar HTML

#### ISSUE-14: Tests no corren automáticamente en CI
**Fix**: Crear `.github/workflows/ci.yml`

---

## 🎯 Plan de Implementación (4 semanas)

### 📅 Semana 1: "Stable & Polished" (Días 1-5)

**Objetivo**: Código 100% limpio, .gitignore, magic numbers eliminados

| Tarea | Asignado | TIempo | Estado |
|-------|----------|--------|--------|
| 1.1 Crear .gitignore | 🛠️ Auto | 10min | ❌ |
| 1.2 Añadir constantes `OFFSCREEN_Y` a theme.h | 🛠️ Auto | 30min | ❌ |
| 1.3 Reemplazar literales 30, 4, 2, -10000 en manager/input.c | 🛠️ Auto | 1h | ❌ |
| 1.4 Reemplazar colores hardcoded en render.c por theme.h | 🛠️ Auto | 1h | ❌ |
| 1.5 Añadir logging niveles configurables (tui_logger_set_level) | 🛠️ Auto | 2h | ❌ |
| 1.6 Verificar build `-Werror` tras cambios | 🛠️ Auto | 30min | ❌ |
| 1.7 Commit + tag v0.1.0 | — | 30min | ❌ |

**Entregables**:
- `.gitignore` committed
- Cero magic numbers literales
- Build passing sin warnings
- Tag `v0.1.0` creado

---

### 📅 Semana 2: "Testing Boost" (Días 6-10)

**Objetivo**: Cobertura > 30%

| Tarea | Asignado | Tiempo | Estado |
|-------|----------|--------|--------|
| 2.1 Escribir `test_tab.c` (create, destroy, add_window, remove_window) | 🛠️ Auto | 3h | ❌ |
| 2.2 Escribir `test_workspace.c` (create, add_tab, set_active) | 🛠️ Auto | 3h | ❌ |
| 2.3 Escribir `test_window.c` (create, destroy, redraw) | 🛠️ Auto | 2h | ❌ |
| 2.4 Mock de notcurses para tests (ncmock.h) | 🛠️ Auto | 4h | ❌ |
| 2.5 Integrar lcov/gcov para coverage | 🛠️ Auto | 1h | ❌ |
| 2.6 `make test` debe reportar % coverage | — | 30min | ❌ |

**Entregables**:
- 5+ tests unitarios nuevos
- Coverage report HTML en `build/coverage/`
- Cobertura > 30%

---

### 📅 Semana 3: "Encapsulation & API" (Días 11-15)

**Objetivo**: API estable y documentada

| Tarea | Asignado | Tiempo | Estado |
|-------|----------|--------|--------|
| 3.1 Convertir `TuiTab`, `TuiWorkspace`, `TuiWindow` a tipos opacos en headers | 🛠️ Auto | 4h | ❌ |
| 3.2 Añadir getters públicos (name, id, window_count, etc.) | 🛠️ Auto | 3h | ❌ |
| 3.3 Revisar consistencia de API (const correcto, naming) | 🛠️ Auto | 2h | ❌ |
| 3.4 Añadir documentación Doxygen a headers | 🛠️ Auto | 4h | ❌ |
| 3.5 Generar HTML de docs y subir a GitHub Pages | 🛠️ Auto | 1h | ❌ |

**Entregables**:
- API completamente opaca (excepto `types.h` interno)
- Doxygen configurado, HTML generado
- Breaking changes documentados en CHANGELOG

---

### 📅 Semana 4: "CI/CD & Release" (Días 16-20)

**Objetivo**: Pipeline automatizado y release candidate

| Tarea | Asignado | Tiempo | Estado |
|-------|----------|--------|--------|
| 4.1 GitHub Actions: build, test, lint, coverage | 🛠️ Auto | 3h | ❌ |
| 4.2 GitHub Actions: ASAN + valgrind check | 🛠️ Auto | 2h | ❌ |
| 4.3 Añadir `make install`, `make uninstall` | 🛠️ Auto | 2h | ❌ |
| 4.4 Crear `pkg-config` file `tmlcs-tui.pc` | 🛠️ Auto | 1h | ❌ |
| 4.5 Preparar release v1.0.0rc1 | — | 2h | ❌ |
| 4.6 Escribir CHANGELOG.md y UPDATE.md | — | 2h | ❌ |

**Entregables**:
- CI/CD completo en GitHub Actions (verde en todas las matrices)
- `make install` funcional
- Release candidate publicado

---

## 📈 Métricas de Éxito

| Métrica | Current (hoy) | Target (v1.0.0rc1) |
|---------|---------------|--------------------|
| **Score Global** | 7.0/10 | 8.5/10 |
| **Cobertura de tests** | ~2% | > 40% |
| **Magic numbers literales** | ~15 | 0 |
| **API opacity** | Tipos transparentes | Tipos opacos |
| **CI/CD** | No | ✅ GitHub Actions |
| **Documentación** | 0% | > 80% API cubierta |
| **Build warnings** | 0 ✅ | 0 ✅ |

---

## 🚀 Quick Wins (Implementar HOY — < 2h)

### 1. **.gitignore** — 2 minutos
```gitignore
# Build artifacts
/build/
/mi_tui
*.o
*.a
*.so
*.dylib

# IDE / Editors
.vscode/
.idea/
*.swp
*.swo

# OS generated
.DS_Store
Thumbs.db

# Logs
*.log
/tmp/

# Test coverage
coverage/
*.gcda
*.gcno
```

### 2. **Sanitizers opcionales** — 5 minutos
Añadir a Makefile:
```makefile
CFLAGS_SAN = -fsanitize=address,undefined
LDFLAGS_SAN = -fsanitize=address,undefined

asan: CFLAGS += $(CFLAGS_SAN)
asan: LDFLAGS += $(LDFLAGS_SAN)
asan: clean all

.PHONY: asan
```
Uso: `make asan && ASAN_OPTIONS=detect_leaks=1 ./mi_tui`

### 3. **Constantes offscreen** — 15 minutos
En `include/core/theme.h`:
```c
#define OFFSCREEN_Y -10000  // Off-screen parking para planos ocultos
```
Luego reemplazar en todos los archivos.

### 4. **Asegurar NULL check en todos los malloc/calloc** — 20 minutos
Verificar:
- `tui_tab_create` → ya ✅
- `tui_workspace_create` → ya ✅
- `tui_window_create` → ya ✅
- `tui_manager_create` → ya ✅ (pero `realloc` sin check en state.c:47)

**Fix state.c:47-49**:
```c
TuiWorkspace** new_ws = realloc(manager->workspaces, ...);
if (!new_ws) {
    tui_log(LOG_ERROR, "OOM en tui_manager_add_workspace (realloc)");
    return;
}
manager->workspaces = new_ws;
```

---

## 📋 Checklist de Implementación (Para git commit)

```
[ ] .gitignore agregado y commiteado
[ ] OFFSCREEN_Y definido en theme.h
[ ] Off-screen literales -10000 reemplazados por OFFSCREEN_Y
[ ] Tab bar offset 30 se usa TAB_BAR_START_X (ya definido)
[ ] manager/input.c no tiene magic numbers 30, 4, 2 residuales
[ ] manager/render.c no tiene colores hardcoded (usar theme.h)
[ ] Build: make clean && make && make test (0 warnings)
[ ] Tests unitarios: test_tab.c, test_workspace.c, test_window.c
[ ] Coverage > 30%
[ ] Tipos opacos en headers públicos
[ ] Doxygen configurado y HTML generado
[ ] GitHub Actions configurado
[ ] Tag v1.0.0rc1 creado
```

---

## 📚 Recursos y Referencias

### Herramientas de Calidad
- `clang-tidy` (ya configurado con `.clang-tidy`)
- `valgrind --leak-check=full ./mi_tui`
- `ASAN_OPTIONS=detect_leaks=1 ./mi_tui`
- `lcov` / `gcov` para coverage
- `doxygen Doxyfile` para docs

### Comandos Útiles
```bash
# Build completo con warnings como errores
make clean && make

# Tests con coverage
make clean
CFLAGS="--coverage" make
./build/test_runner
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage/

# CI local simulation
docker run --rm -v $(pwd):/src -w /src ubuntu:22.04 bash -c "
  apt-get update && apt-get install -y build-essential libnotcurses-dev cmake
  make clean && make && make test
"

# Sanitizers
make clean && make asan
ASAN_OPTIONS=detect_leaks=1 ./mi_tui
```

---

## 🔄 Proceso de Release

### v1.0.0rc1 (Objetivo: fin de semana 4)
- ✅ CI/CD passing en todas las matrices (Ubuntu, macOS)
- ✅ Memory leak report: 0 leaks
- ✅ Coverage report >= 40%
- ✅ Docs actualizadas
- ✅ Changelog completo depuis v0.1.0
- ✅ GitHub Release draft con assets:
  - `tmlcs-tui-1.0.0rc1.tar.gz` (source)
  - `coverage.html` (zip)

---

## ⚠️ Notas Importantes

1. **No romper API existente**: Si cambios enñan, documentar `CHANGELOG.md`
2. **Tests first**: Escribir tests antes de refactorizar módulos
3. **Commit granular**: Un cambio por commit, mensajes descriptivos
4. **Branch strategy**: `main` → estable, `dev` → desarrollo, `feature/*` → features

---

*Fin del Plan de Calidad*

**Próximo paso inmediato**: Implementar Quick Wins (sección superior) y hacer `git commit -m "chore: quality baseline"` para tener punto de partida.
