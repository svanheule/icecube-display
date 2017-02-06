include <front.scad>;
include <back_bottom.scad>;
include <back_top.scad>;

split = 3;

translate(v=[(base_width+split)/2, (base_length+split)/2, 0]) {front();}
translate(v=[-(base_width+split)/2, (base_length+split)/2, 0]) {front();}
translate(v=[(base_width+split)/2, -(base_length+split)/2, 0]) {back_top();}
translate(v=[-(base_width+split)/2, -(base_length+split)/2, 0]) {back_bottom();}
