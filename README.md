## NoBuzz
NoBuzz is a free laptop buzzing/mosquito noise removal utility.

The high pitch, tinny mosquito-like noise is often caused by applications like Chrome and Flash which increase system timer resolution to 1ms, which causes some aging PC systems (especially laptops) to buzz at 1KHz frequency. This very distinctive noise is often referred to as [coil noise](https://en.wikipedia.org/wiki/Coil_noise) because it is produced by coils in the VR circuit.

### How does it work?

NoBuzz attempts to prevent calls to `timeBeginPeriod(uPeriod)` with uPeriod value <16. This is often sufficient to stop the mosquito noise.

Note: NoBuzz is _not a cure_ for the coil noise issue, it is merely a workaround. Side effects include:
* Animations might be less smooth
* Some applications might not function properly (please open a ticket if you find one)
* Not recommended for game systems

Note that this tool has no effect on GPU coil whine, it works only on CPU sleep state noise. Note also that there are other ways to increase system timer resolution, and some applications (e.g. virtualization software) will still be able to do that and consequently the noise will remain.

### Installation

Download the latest [setup file](https://github.com/rustyx/nobuzz/releases/download/0.6/nobuzz_setup.exe), install, reboot (or log off/on), enjoy!

Note: if the mosquito noise still remains, `powercfg -energy` might be able to provide an answer on which app increased the timer resolution.

### How can I help?

Use it, star it, share it and let me know if it worked for you!
