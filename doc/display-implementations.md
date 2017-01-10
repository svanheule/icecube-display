# List of implementations {#display_implementations}
This page lists the firmware implementations contained in this icecube-display repository.

<TABLE>
  <TR>
    <TH>Directory</TH>
    <TH>Description</TH>
    <TH>Serial number</TH>
    <TH>USB product ID</TH>
  </TR>
  <TR>
    <TD>\subpage display_atmega32u4_icetop "icetop_atmega32u4"</TD>
    <TD>Table-top IceTop display</TD>
    <TD>ICD-IT-001-xxxx</TD>
    <TD>0x0001</TD>
  </TR>
  <TR>
    <TD>\subpage display_teensy_icecube "icecube_teensy32"</TD>
    <TD>Modular \f$(2m)^3\f$ IceCube display</TD>
    <TD>ICD-IC-001-xxxx</TD>
    <TD>0x0002</TD>
  </TR>
</TABLE>

## Serial number
The serial number is a 15 character string with "ICD-zz-yyy-xxxx" as format:
* __zz__: 'IT' or 'IC', depending on whether the device is meant to display IceTop or IceCube.
* __yyy__: Sequential number indicating the display model. All displays with the same zz-yyy value
    should be able to use the same device firmware.
* __xxxx__: Sequential number to provide a unique identifier for the hardware.
