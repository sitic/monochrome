```{toctree}
:maxdepth: 3
:hidden:

Introduction & Installation <self>
```

```{toctree}
:maxdepth: 2
:hidden:

API Reference <api/monochrome>
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
