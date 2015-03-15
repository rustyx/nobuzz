## NoBuzz
NoBuzz is a computer buzzing/mosquito noise removal utility.

The noise is often caused by applications like Chrome, Flash etc. that increase system timer resolution to 1ms,
which causes some aging PC systems (especially laptops) to buzz at 1KHz.

NoBuzz attempts to prevent calls to `timeBeginPeriod(uPeriod)` with uPeriod value <16. This is often sufficient to stop the buzzing noise.

Note: NoBuzz is _not a cure_ for the buzzing issue, it is merely a workaround. The side effects include:
* Animations might be less smooth
* Some applications might not function properly (please open a ticket if you find one)
* Not recommended for game systems

Note also that there are other ways to increase system timer resolution, e.g. via a device driver, and some applications (e.g. virtualization software) will still be able to do that.
