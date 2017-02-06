This directory contains design files for the modular IceCube
display.

* `crates`: Dimensions of plywood pieces used to construct the different crates.
    All dimensions are expressed in centimeter, and were converted from a design
    with dimensions in inches.
    These designs assume a plate thickness of 12.7mm (0.5 inch), but the plate
    thickness used was 5.5mm, to reduce weight and costs. The pre-calculated
    dimensions were adapted as needed on the go, and the milled-out slots were
    replaced by -- easier to produce -- drilled out holes of similar diameter.
* `pcb`: KiCad design files for
  * `data_splitter`: PCB used to change from one 8P8C (RJ45) connection to 4
    4P4C connections, splitting the four pairs in an ethernet cables.
  * `string_adapter`: PCB used to insert power and data into the attachment
    points for the LED strips.
  * `libraries`: common footprints, etc.
* `strip_suspension_105mm.dxf`: Drawing for laser-cutting the PMMA strips used
  to suspend the LED strips.
* `strip_clip`: OpenSCAD design files for the clips that were put on the LED
  strips.
  The suspension strip can be inserted in the top clip, the bottom clip can
  be used to attach to a loop of elastid cord. This will put some tension
  on the strip, straightening it out.
