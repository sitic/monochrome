import setuptools

setuptools.setup(
    name='quickVidViewer',
    version='0.0.1',
    url='https://gitlab.gwdg.de/jlebert/quickmultrecviewer',
    classifiers=[
        "Private :: Do not Upload"
    ],
    python_requires='>=3.6',
    packages=setuptools.find_packages(),
    install_requires=['numpy', 'flatbuffers'],
)
