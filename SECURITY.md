# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 0.5.x   | :white_check_mark: |
| < 0.5   | :x:                |

## Reporting a Vulnerability

If you discover a security vulnerability in tmlcs-tui, please report it
privately via the GitHub Security tab or by opening an issue marked [SECURITY].

Please include:
- A description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

We will respond within 48 hours and aim to publish a fix within 14 days.

## Security Considerations

tmlcs-tui processes untrusted terminal input. Key security boundaries:
- Text input widgets handle arbitrary UTF-8 sequences
- File picker widget reads filesystem paths
- Logger handles untrusted strings
- Clipboard integration sends/receives data from terminal

All buffer operations use bounds-checked functions (snprintf, strncpy, memmove).
The UTF-8 subsystem validates codepoint boundaries.
