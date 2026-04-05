# 🔍 TMLCS-TUI — Auditoría de Calidad y Plan de Mejora

## 1. Resumen Ejecutivo

| **Aspecto** | **Descripción** |
|---|---|
| **Proyecto** | TMLCS-TUI — Terminal User Interface con arquitectura de Workspace → Tab → Window |
| **Lenguaje** | C17 (implícito) |
| **Framework de UI** | notcurses |
| **Build System** | GNU Make (Makefile) |
| **Tests** | Unity framework, 1 archivo de test (`test_logger.c`) |
| **Estado de la Build** | ✅ Compila exitosamente |
| **Tests** | ⚠️ Parciales (solo logger, falla por `__func__` vs `__FUNCTION__`) |

---

## 2. Evaluación por Categoría

### 2.1 📐 Arquitectura y Diseño de Modularidad

| Métrica | Estado | Detalle |
|---------|--------|---------|
| **Arquitectura** | ✅ Buena | Separación clara: `include/` (API) / `src/` (implementación) |
| **Encapsulamiento de tipos** | ✅ Correcto | Forward declarations en `types.h`, uso de `typedef` |
| **Acoplamiento entre módulos** | ⚠️ Alto | Los componentes acceden directamente a campos internos (`tab->window_count`, `ws->tab_count`) |
| **"Manager" directory** | ❌ Crítico | `src/core/manager/` existe pero está **vacío**. Funciones `tui_manager_*` están comentadas en `src/main.c` |
| **Módulo `tab.c`** | ⚠️ Inconsistente | Contiene funciones de `Workspace`, `Window` y `Manager` mezcladas en un solo archivo (434 LOC vs ~50 LOC declarados en header) |

### 2.2 🛡️ Seguridad de Memoria y Manejo de Recursos

| Métrica | Estado | Detalle |
|---------|--------|---------|
| **Gestión de memoria** | ❌ Crítico | Uso extenso de `malloc` sin validación de retorno NULL |
| **Buffer safety** | ❌ Crítico | `sprintf()` sin limites (ej: `sprintf(tab->name, ...)`). Reemplazar con `snprintf()` |
| **Memory leaks** | ⚠️ Alto | `tui_tab_create` y `tui_workspace_create` liberan memoria parcialmente en error paths |
| **NULL pointer checks** | ⚠️ Inconsistente | Algunas funciones validan NULL, otras no (ej: `tui_window_destroy` no valida NULL antes de acceder) |
| **Dangling pointers** | ⚠️ Alto | `tui_tab_remove_window` usa `memmove` sin invalidar el puntero eliminado ni actualizar referencias |

### 2.3 💅 Código Limpio y Estilo

| Métrica | Estado | Detalle |
|---------|--------|---------|
| **Código comentado** | ❌ Crítico | ~60% del código está comentado con `// [Comentado ...]` y bloques `#if 0` |
| **Funciones no utilizadas** | ⚠️ Alto | `tui_workspace_create`, `tui_workspace_destroy`, `tui_tab_rename` declaradas pero no usadas |
| **Logging en producción** | ⚠️ Medio | `tui_log` imprime a `stderr` sin mechanismo para silenciar/redirigir en producción |
| **Magic numbers** | ✅ Buena mejora | Centralizados en `theme.h` (87 LOC de constantes bien organizadas) |
| **Documentación** | ⚠️ Medio | `ACTION_PLAN.md` existe pero sin actualizar con la evolución real del proyecto |

### 2.4 🔧 Build System y Configuración

| Métrica | Estado | Detalle |
|---------|--------|---------|
| **Makefile** | ⚠️ Medio | Falta target `install`, `test` no es parte del flujo CI |
| **Warnings del compilador** | ✅ Buena | `-Wall -Wextra` habilitados |
| **Falta** | ❌ Crítico | Falta `-Werror`, `-pedantic`, sanitizers (`-fsanitize=address,undefined`) |
| **Target de limpieza** | ✅ Buena | `clean` elimina `build/` completo |
| **Dependencias** | ⚠️ Medio | `notcurses` requerida pero sin verificación en el Makefile |

### 2.5 🧪 Testing y Cobertura

| Métrica | Estado | Detalle |
|---------|--------|---------|
| **Cobertura** | ❌ Crítico | 1 test (`test_logger.c`) cubre ~2% del código |
| **Framework de testing** | ⚠️ Medio | Unity es apropiado, pero `__func__` vs `__FUNCTION__` causa incompatibilidad |
| **CI/CD** | ❌ No presente | Sin GitHub Actions ni pipeline de validación |
| **Test de integración** | ❌ No presente | Sin tests de UI, sin mock de notcurses |
| **Asserts** | 🟣 Parcial | Solo en `test_logger.c`, no en código principal |

### 2.6 📝 Headers y API Pública

| Métrica | Estado | Detalle |
|---------|--------|---------|
| **Include guards** | ✅ Correcto | Todos los headers usan `#ifndef` guards |
| **API consistente** | ⚠️ Medio | Nomenclatura inconsistente: `tui_window_create` vs `tui_tab_create` (parámetros diferentes) |
| **Parámetros `const`** | ❌ Deficiente | Pocos parámetros marcados como `const` cuando deberían serlo |
| **Documentación de API** | ❌ No presente | Sin comentarios Doxygen o similares |

---

## 3. Issues Críticos Detallados

### 🔴 CRIT-01: Código Comentado Masivo
**Archivo**: `src/core/tab.c`  
**Impacto**: Imposible mantener código con 60% comentado. Crea confusión y bugs silenciosos.  
**Ejemplos**:
- Líneas 17-331: Funciones enteras comentadas con `[Comentado para no eliminar por ahora]`
- `#if 0` blocks desde línea 382 hasta ~600
- Código duplicado: implementaciones vivas y comentadas de la misma funcionalidad

### 🔴 CRIT-02: Buffer Overflows con `sprintf`
**Ubicación**: `src/core/tab.c:85-88`  
**Riesgo**: Escritura fuera de bounds si nombres exceden el buffer de 64 bytes.  
**Recomendación**: Reemplazar TODOS los `sprintf` con `snprintf`:
```c
sprintf(tab->name, "Tab %d", tab->id);  // ❌ PELIGROSO
snprintf(tab->name, sizeof(tab->name), "Tab %d", tab->id);  // ✅ SEGURO
```

### 🔴 CRIT-03: Manager Vacío
**Directorio**: `src/core/manager/` (vacío)  
**Impacto**: La pieza central de la arquitectura no existe. Las funciones están comentadas en `main.c`.  
**Recomendación**: Crear `manager.c` con funciones `tui_manager_*` activas o eliminar el concepto de manager.

### 🔴 CRIT-04: Memory Leaks en Error Paths
**Ubicación**: `src/core/tab.c:72-76`, `src/core/tab.c:137-141`  
**Problema**: Si falla la segunda `malloc`, la primera ya asignada no se libera correctamente en todos los casos.

### 🔴 CRIT-05: Función `tui_tab_remove_window` Destructiva
**Ubicación**: `src/core/tab.c:373-378`  
**Problema**: Usa `memmove` pero no libera memoria de la ventana eliminada ni actualiza `active_window_index`.

---

## 4. 🎯 Plan de Acción Priorizado

### 🚨 Semana 1: Estabilización Crítica (Bloqueantes)

| # | Acción | Esfuerzo | Resultado Esperado |
|---|--------|----------|-------------------|
| 1 | Eliminar TODOS los bloques de código comentado en `tab.c` (~300 líneas muertas) | 2h | Código legible y mantenible |
| 2 | Reemplazar `sprintf` → `snprintf` en todo el proyecto | 1h | Eliminar riesgo de buffer overflow |
| 3 | Validar `malloc != NULL` en todas las llamadas | 1h | Crash prevention |
| 4 | Crear `manager.c` funcional o eliminar el concepto | 4h | Arquitectura consistente |
| 5 | Hacer `clean` + `make` exitoso con warnings limpios | 2h | `-Wall -Wextra -Werror` sin errores |

### ⚡ Semana 2: Mejora de Robustez

| # | Acción | Esfuerzo | Resultado Esperado |
|---|--------|----------|-------------------|
| 1 | Implementar sanitizers en Makefile: `-fsanitize=address,undefined` | 1h | Detección automática de bugs de memoria |
| 2 | Añadir `const` a parámetros de funciones que no modifican datos | 2h | Seguridad de tipos |
| 3 | Revisar y corregir `tui_tab_remove_window` | 2h | Eliminar dangling pointers |
| 4 | Corregir test `test_logger.c` (`__func__` vs `__FUNCTION__`) | 30min | Tests pasando |
| 5 | Añadir tests para `tui_tab_create`, `tui_workspace_create` | 3h | Cobertura > 10% |

### 🔬 Semana 3: Testing y Automatización

| # | Acción | Esfuerzo | Resultado Esperado |
|---|--------|----------|-------------------|
| 1 | Configurar GitHub Actions (build + test en push) | 2h | CI/CD operativo |
| 2 | Añadir tests de edge cases (NULL inputs, bounds) | 4h | Cobertura > 30% |
| 3 | Implementar `tui_log` con niveles y redirección configurable | 3h | Logging de producción |
| 4 | Documentar API pública con comentarios Doxygen | 4h | Documentación generada |

### 🏗️ Semana 4+: Refactorización Estructural

| # | Acción | Esfuerzo | Resultado Esperado |
|---|--------|----------|-------------------|
| 1 | Separar `tab.c` en archivos: `tab.c`, `workspace.c`, `window.c`, `manager.c` | 8h | Arquitectura modular real |
| 2 | Implementar getters/setters para encapsular campos internos | 4h | Bajo acoplamiento |
| 3 | Añadir `make install`, `make uninstall`, pkg-config | 2h | Instalación profesional |
| 4 | Implementar Valgrind/ASAN CI check | 2h | Detección de memory leaks |

---

## 5. 📊 Scorecard de Calidad

| Categoría | Score | Peso | Ponderado |
|-----------|-------|------|-----------|
| Arquitectura | 5/10 | 20% | 1.0 |
| Seguridad de Memoria | 3/10 | 25% | 0.75 |
| Código Limpio | 3/10 | 15% | 0.45 |
| Build System | 6/10 | 10% | 0.6 |
| Testing | 2/10 | 20% | 0.4 |
| Documentación | 3/10 | 10% | 0.3 |
| **TOTAL** | | **100%** | **3.5/10** |

**Estado General**: 🟡 **En Desarrollo** — Requiere trabajo significativo para alcanzar calidad de producción.

---

## 6. 🚀 Quick Wins (Implementar Inmediatamente)

1. **Añadir sanitizers al Makefile**: Solo 2 líneas añadidas a `CFLAGS`:
   ```makefile
   CFLAGS = -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude -g
   LDFLAGS = -fsanitize=address,undefined -lnotcurses -lnotcurses-core -pthread
   ```

2. **Crear `.gitignore`** para archivos generados:
   ```
   build/
   mi_tui
   *.o
   ```

3. **Eliminar código comentado** — El impacto es inmediato y masivo en legibilidad.

4. **Añadir validación de NULL** pattern:
   ```c
   if (!ptr) {
       tui_log_error("%s: NULL pointer", __func__);
       return NULL;
   }
   ```

---

*Auditoría generada: 2026-02-19*  
*Versión del proyecto: v0.1-alpha*
