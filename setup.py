from skbuild import setup

setup(
    name='monochrome',
    # version='0.2.0',
    url='https://gitlab.com/cardiac-vision/monochrome',
    author="Jan Lebert",
    author_email="jan.lebert@ucsf.edu",
    classifiers=[
        "Private :: Do not Upload",
        "Development Status :: 3 - Alpha",
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
    install_requires=['numpy', 'flatbuffers>=2.00'],
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
    zip_safe=False
)
