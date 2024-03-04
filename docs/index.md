```{toctree}
:maxdepth: 2
:hidden:

Introduction <self>
installation_standalone
installation_python
tutorial
development
Python API Reference <api/monochrome>
```

% Generate API reference
```{eval-rst}
.. autosummary::
   :toctree: api
   :template: custom-module-template.rst
   :recursive:
   :hidden:

   monochrome
```

```{include} ../README.md
:relative-docs: docs/
:relative-images:
```
