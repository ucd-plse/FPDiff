cd /usr/local/src/fp-diff-testing/workspace && make reset

python3 /usr/local/src/fp-diff-testing/workspace/extractor.py mpmath /usr/local/lib/python3.6/dist-packages/mpmath/tests/ python >> /usr/local/src/fp-diff-testing/workspace/logs/__extractedSignatures.txt
python3 /usr/local/src/fp-diff-testing/workspace/extractor.py scipy /usr/local/lib/python3.6/dist-packages/scipy/special/tests/ python >> /usr/local/src/fp-diff-testing/workspace/logs/__extractedSignatures.txt
python3 /usr/local/src/fp-diff-testing/workspace/extractor.py gsl /usr/local/lib/gsl/specfunc/ c >> /usr/local/src/fp-diff-testing/workspace/logs/__extractedSignatures.txt
python3 /usr/local/src/fp-diff-testing/workspace/extractor.py jmat /usr/local/lib/jmat/ javascript >> /usr/local/src/fp-diff-testing/workspace/logs/__extractedSignatures.txt

python3 /usr/local/src/fp-diff-testing/workspace/driverGenerator.py mpmath python
python3 /usr/local/src/fp-diff-testing/workspace/driverGenerator.py scipy python
python3 /usr/local/src/fp-diff-testing/workspace/driverGenerator.py gsl c
python3 /usr/local/src/fp-diff-testing/workspace/driverGenerator.py jmat javascript

python3 /usr/local/src/fp-diff-testing/workspace/classify.py

python3 /usr/local/src/fp-diff-testing/workspace/utils/generateSpecialValueInputs.py

python3 /usr/local/src/fp-diff-testing/workspace/diffTester.py
