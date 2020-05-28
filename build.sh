#!/usr/bin/env bash

# install mpmath
pip3 install mpmath==1.1.0

# install scipy
pip3 install scipy==1.3.1

# install pytest
pip3 install pytest

# install selenium
pip3 install selenium

# git clone jmat
cd /usr/local/lib
git clone https://github.com/lvandeve/jmat.git
cd /usr/local/lib/jmat
git checkout 21d15fc3eb5a924beca612e337f5cb00605c03f3

# begin install gsl
cd /usr/local/lib
wget ftp://ftp.gnu.org/gnu/gsl/gsl-2.6.tar.gz
tar -xvf gsl-2.6.tar.gz
rm gsl-2.6.tar.gz
mv gsl-2.6 gsl

cd /usr/local/lib/gsl
mkdir gsl-library
./configure --prefix=/usr/local/lib/gsl/gsl-library
make
make install

cd /usr/local/lib/gsl/gsl-library/lib
rm libgslcblas.so.0
rm libgslcblas.so
rm libgsl.so
rm libgsl.so.25
mv libgslcblas.so.0.0.0 libgslcblas.so.0
mv libgsl.so.25.0.0 libgsl.so.23
# end install gsl

cd /usr/local/src/fp-diff-testing

rm build.sh