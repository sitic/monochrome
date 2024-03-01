from skbuild import setup

setup(
    name='monochrome-viewer',
    url='https://gitlab.com/sitic/monochrome',
    author="Jan Lebert",
    author_email="mail@janlebert.com",
    license="MIT",
    description="Viewer for scientific monochromatic video data",
    long_description=open("README.md", encoding="utf8").read(),
    long_description_content_type="text/markdown",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: MacOS",
        "Operating System :: POSIX",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3",
        "Topic :: Desktop Environment",
        "Topic :: Multimedia :: Video :: Display",
        "Topic :: Scientific/Engineering :: Visualization"
    ],
    python_requires='>=3.6',
    packages=['monochrome', 'monochrome.fbs'],
    package_dir={
        'monochrome': 'src/python/monochrome',
        'monochrome.fbs': 'src/python/monochrome/fbs'
    },
    tests_require=['pytest'],
    install_requires=['numpy', 'flatbuffers>=23.5.26'],
    entry_points={
        'console_scripts': [
            'Monochrome=monochrome.ipc:console_entrypoint'
        ]
    },
    cmake_source_dir='.',
    cmake_args=[
        '-DSTATIC_GCC:BOOL=ON',
        '-DDISABLE_MACOS_BUNDLE:BOOL=ON',
        ],
    cmake_install_dir='src/python/monochrome/data',
    zip_safe=False,
    extras_require={
        "docs": [
            "sphinx",
            "sphinxcontrib-napoleon",
            "sphinxcontrib-bibtex",
            "sphinxcontrib-video",
            "sphinx-autobuild",
            "sphinx-copybutton",
            "sphinx-codeautolink",
            "furo",
            "myst_nb>=1.0.0",
            "jupytext",
            "jupyter-cache",
            "sphinx-remove-toctrees",
            "sphinx-design",
            "enum-tools[sphinx]",
            "opticalmapping[all]"
        ],
    },
)
