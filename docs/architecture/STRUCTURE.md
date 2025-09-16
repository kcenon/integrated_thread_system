# Integrated Thread System - New Structure

## Directory Layout

```
integrated_thread_system/
├── include/kcenon/integrated/   # Public headers
│   ├── framework/              # Application framework
│   ├── patterns/               # Integration patterns
│   └── utils/                  # Utilities
├── src/                        # Implementation
│   ├── framework/              # Framework implementation
│   ├── patterns/               # Pattern implementations
│   └── utils/                  # Utility implementations
├── tests/                      # All tests
│   ├── unit/                   # Unit tests
│   ├── integration/            # Integration tests
│   └── system/                 # System tests
├── examples/                   # Usage examples
│   ├── basic/                  # Basic usage
│   ├── advanced/               # Advanced scenarios
│   └── patterns/               # Pattern demonstrations
├── config/                     # Configuration files
│   └── default.json           # Default configuration
├── docs/                       # Documentation
└── CMakeLists.txt             # Build configuration
```

## Namespace Structure

- Root: `kcenon::integrated`
- Framework: `kcenon::integrated::framework`
- Patterns: `kcenon::integrated::patterns`
- Utilities: `kcenon::integrated::utils`

## Dependencies

The integrated system depends on:
- `kcenon::thread` - Thread system
- `kcenon::logger` - Logger system
- `kcenon::monitoring` - Monitoring system

## Key Components

### Application Framework
- Unified initialization and lifecycle management
- Automatic component wiring and configuration
- Graceful shutdown handling
- Health monitoring

### Integration Patterns
- Monitored task execution
- Logged operations
- Performance tracked workflows
- Circuit breakers and retry policies

### Configuration
- JSON-based configuration
- Environment-specific profiles
- Runtime configuration updates
- Validation and defaults

## Usage Example

```cpp
#include <kcenon/integrated/framework/application.h>

using namespace kcenon::integrated::framework;

int main() {
    application::config config;
    config.config_file_path = "config/production.json";

    application app(config);

    if (app.initialize()) {
        app.run();
    }

    return 0;
}
```

## Migration Notes

1. Update all namespace references using `./migrate_namespaces.sh`
2. Update CMakeLists.txt for new structure
3. Update include paths in dependent projects
4. Test all integration points
