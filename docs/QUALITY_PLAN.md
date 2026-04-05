# 📋 TMLCS-TUI — Plan de Calidad (4 semanas)

## Fase 1: Estabilización (d1-5)
• Limpiar código comentado en tab.c
• sprintf → snprintf
• NULL checks en malloc
• manager.c funcional o eliminar
• Build -Werror limpio

## Fase 2: Robustez (d6-10)
• Sanitizers: -fsanitize=address,undefined
• const correctness
• Corregir tui_tab_remove_window
• Tests: 20% cobertura

## Fase 3: CI/CD (d11-15)
• GitHub Actions (.github/workflows/ci.yml)
• Logging configurable por niveles
• Documentación Doxygen

## Fase 4: Refactorización (d16-20)
• Separar: tab, window, workspace, manager
• Encapsulación con getters/setters
• ASAN/Valgrind en CI

Métricas: 3.5/10 → 8/10
