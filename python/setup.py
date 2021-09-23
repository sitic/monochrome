import setuptools

setuptools.setup(
    name='monochrome',
    version='0.2.0',
    url='https://gitlab.com/cardiac-vision/monochrome',
    classifiers=[
        "Private :: Do not Upload"
    ],
    python_requires='>=3.6',
    packages=setuptools.find_packages(),
    install_requires=['numpy', 'flatbuffers>=2.00'],
)
