## NoBuzz
NoBuzz is a free computer buzzing/mosquito noise removal utility.

The noise is often caused by applications like Chrome and Flash which increase system timer resolution to 1ms,
which causes some aging PC systems (especially laptops) to buzz at 1KHz.

### How does it work?

NoBuzz attempts to prevent calls to `timeBeginPeriod(uPeriod)` with uPeriod value <16. This is often sufficient to stop the mosquito noise.

Note: NoBuzz is _not a cure_ for the buzzing issue, it is merely a workaround. Side effects include:
* Animations might be less smooth
* Some applications might not function properly (please open a ticket if you find one)
* Not recommended for game systems

Note also that there are other ways to increase system timer resolution, and some applications (e.g. virtualization software) will still be able to do that.

### Installation

Download the latest [setup file](https://github.com/rustyx/nobuzz/releases/download/v0.5/nobuzz_setup.exe), install, reboot (or log off/on), enjoy!

Note: if the mosquito noise still remains, `powercfg -energy` might be able to provide an answer on which app increased the timer resolution.

### How can I help?

Use it, star it, share it and let me know if it worked for you!
