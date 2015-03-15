## NoBuzz
NoBuzz is a computer buzzing/mosquito noise removal utility.

The noise is often caused by applications like Chrome, Flash etc. that increase system timer resolution to 1ms,
which causes some aging PC systems (especially laptops) to buzz at 1KHz.

NoBuzz attempts to prevent calls to `timeBeginPeriod()` with a parameter value <16.

Note: NoBuzz is **not a cure** for the buzzing issue, it is merely a workaround. The side effects include:
* animations might be less smooth
* some applications might not function properly
