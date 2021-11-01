from skbuild import setup

setup(
    name='monochrome',
    # version='0.2.0',
    url='https://gitlab.com/cardiac-vision/monochrome',
    classifiers=[
        "Private :: Do not Upload"
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
            'Monochrome=monochrome:monochrome'
        ]
    },
    cmake_source_dir='.',
    cmake_args=['-DSTATIC_GCC:BOOL=ON'],
    cmake_install_dir='src/python/monochrome/data',
)
