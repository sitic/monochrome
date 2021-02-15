import setuptools

setuptools.setup(
    name='monochrome',
    version='0.1.0',
    url='https://gitlab.gwdg.de/lebert/Monochrome',
    classifiers=[
        "Private :: Do not Upload"
    ],
    python_requires='>=3.6',
    packages=setuptools.find_packages(),
    install_requires=['numpy', 'flatbuffers>=1.12'],
)
