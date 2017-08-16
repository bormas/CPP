This programm identifies images according to model (data\multiclass\test\...)
The model is trained according to given examples (data\multiclass\train\...)

Unzip data.rar to project folder (these are images to process)

To build project input in console:
"make all"

To teach model input in console from .../build/bin directory:
"task2.exe -d ../../data/multiclass/train_labels.txt -m model.txt --train"

To classify images according to model input in console from .../build/bin directory:
"task2.exe -d ../../data/multiclass/test_labels.txt -m model.txt -l predictions.txt --predict"

To check the precision of classification from project directory:
"./compare.py data/multiclass/test_labels.txt build/bin/predictions.txt"
