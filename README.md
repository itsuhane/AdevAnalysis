# AdevAnalysis
Allan deviation for IMU/MEMS intrinsic calibration.

Usage: AdevAnalysis <datafile> <frequency>

Put sensor readings in a text file and run this code against it.
It compile and runs in Visual Studio, since I used PPL from VS...

This code uses GNUPlot, just install it and make sure its `bin\` directory is added to your `PATH`.

## Input

Put your data in a text file:

```
0.15124
0.10933
0.13794
...
```

Only 1 column of data is allowed. It should be measured at fixed frequency.

## To be Implemented
* replace PPL
* multiple columns
* timestamp
* outlier removal
