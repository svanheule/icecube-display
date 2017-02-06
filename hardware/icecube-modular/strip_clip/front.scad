include <clamp_common.scad>;

module front() {

epsilon = 2;

translate(v=[-base_width/2, -base_length/2, 0]) {
	cube(size=base_size);
}

module pin() {
	pin_height = plate_thickness;
	pin_radius = pin_radius_base+0.3; // 3mm hole, plus tolerance
	cylinder($fn=8, h=pin_height+epsilon, r=pin_radius);
}


module left_pad() {
	translate(v=[-base_width/2+pad_width/2, 0, base_thickness]) {
		difference() {
			translate(v=[0, 0, strip_thickness/2]) {
				cube(size=[pad_width, base_length, strip_thickness], center=true);
			}
			%pin();
		}
	}
}

left_pad();
mirror(v=[1,0,0]) {
	left_pad();
}

}
