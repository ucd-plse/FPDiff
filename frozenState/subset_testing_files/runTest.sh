cd /usr/local/src/fp-diff-testing/workspace

python3 driverGenerator.py mpmath python
python3 driverGenerator.py scipy python
python3 driverGenerator.py gsl c
python3 driverGenerator.py jmat javascript

python3 classify.py

python3 utils/generateSpecialValueInputs.py

python3 diffTester.py
