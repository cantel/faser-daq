# Define module
daqling_module(module_name)

# Add source file to library
daqling_module_sources(${module_name}
    DummyModule.cpp
)

# Provide install target
daqling_module_install(${module_name})
