include <clamp_common.scad>;

module back_top() {

translate(v=[-base_width/2, -base_length/2, 0]) {
	cube(size=base_size);
}

module pin() {
	pin_height = base_thickness - 0.6; // 0.6mm spacing/tolerance
	pin_radius = pin_radius_base; // 3mm hole
	cylinder($fn=8, h=pin_height, r=pin_radius);
}


module left_pin() {
	translate(v=[-base_width/2+pad_width/2, 0, base_thickness]) {
		translate(v=[0, 0, plate_thickness/2]) {
			cube(size=[pad_width, base_length, plate_thickness], center=true);
		}
		translate(v=[0, 0, plate_thickness]) {pin();}
	}
}

left_pin();
mirror(v=[1,0,0]) {
	left_pin();
}

translate(v=[0, 0, base_thickness]) {
	cylinder($fn=20, h=plate_thickness, r=plate_hole_radius);
}

}
