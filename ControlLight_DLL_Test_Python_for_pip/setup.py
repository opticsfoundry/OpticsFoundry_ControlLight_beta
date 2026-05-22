from setuptools import setup
from setuptools.dist import Distribution
import sys

class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True

setup(
    name="control_light_api",
    version="0.1.0",
    author="Your Name",
    description="Python bindings for ControlLight C++ API",
    packages=["control_light_api"],
    package_data={"control_light_api": ["*.pyd", "ControlLight.dll"]},
    include_package_data=True,
    zip_safe=False,
    distclass=BinaryDistribution,
    classifiers=[
        "Programming Language :: Python :: 3.10",
        "Operating System :: Microsoft :: Windows",
    ],
)
