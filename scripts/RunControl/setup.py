from setuptools import find_packages, setup

setup(
    name='daqControl',
    version='1.0.0',
    packages=["daqControl"],
    include_package_data=True,
    zip_safe=False,
    install_requires=[
        'flask',
    ],
)
