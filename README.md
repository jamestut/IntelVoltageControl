# IntelVoltageControl

This command line application allows setting up FIVR voltage offset directly from Microsoft Windows of some Intel CPUs, notably 4th gen (Haswell) and newer.

As this program is command line and applies changes immediately, it is perfect for use as part of automating voltage offset settings, for instance, by using Windows' scheduled tasks. An example use case is that a certain voltage offset might work perfectly on a system when it is being actively used, but might crash the system once it enters modern standby.

Note that this software operates by writing to MSR `0x150`, just like many undervolting utilities do. If other utilities cannot change the voltage offsets, then this software will not work either. In particular, on Skylake based systems with newer firmware that have the "mitigation" for the [Plundervolt](https://plundervolt.com/) vulnerability, the firmware might lock the ability to modify the voltage offset. Some of such firmware allows voltage offset control to be enabled again by disabling both "CFG lock" and "overclocking lock" settings.

## Disclaimer

Since this software modifies hardware parameters, system crash might occur, or in the case of overvolting, system damage.

the software is provided "as is", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. in no event shall the authors or copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or the use or other dealings in the software.

## Usage

This program requires administrator rights.

Put the `WinRing0x64.dll` and `WinRing0x64.sys` (included in the release section of this repository) in the same folder as this program.

- `IntelVoltageControl show`  
  Show all currently configured FIVR offsets.
- `IntelVoltageControl set [--allow-overvolt] [--commit] ([plane_number] [voltage_offset])...`  
  Apply the FIVR offset to the specified control planes.
  - `--allow-overvolt`  
    Specify to set positive voltage offset.
  - `--commit`  
    Actually apply the FIVR offset setting instead of "dry-run".
  - `plane_number`  
    The FIVR control plane number to configure. See next section.
  - `voltage_offset`  
    The voltage offset, in mV (millivolt).

### Example

- To undervolt CPU core and iGPU core by 100 mV and 50 mV respectively:  
  `IntelVoltageControl set --commit 0 -100 1 -50`
- To reset iGPU unslice voltage offset and keep other offsets intact:  
  `IntelVoltageControl set --commit 4 0`

## Control Plane

The FIVR in the Intel CPU regulates voltages for many SoC components. These voltages can be controlled separately by selecting the correct control plane number. A known control plane numbers for Skylake Y/U/H/S are:

- 0  
  CPU core.
- 1  
  Integrated GPU compute unit (slice).
- 2  
  CPU caches.
- 3  
  Skylake system agent, which includes I/O bus and display engine.
- 4  
  Integrated GPU unslice. Contains non-compute GPU parts such as QuickSync, video decoder, and other fixed function hardware.