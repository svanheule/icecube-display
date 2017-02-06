include <clamp_common.scad>;


module back_bottom() {

translate(v=[-base_width/2, -base_length/2, 0]) {
	cube(size=base_size);
}

module pin() {
	pin_height = base_thickness - 0.6; // 0.6mm spacing/tolerance
	pin_radius = pin_radius_base; // 3mm hole
	cylinder($fn=8, h=pin_height, r=pin_radius);
}


module left_pin() {
	translate(v=[-base_width/2+pad_width/2, 0, base_thickness]) {pin();}
}

left_pin();
mirror(v=[1,0,0]) {
	left_pin();
}

}
