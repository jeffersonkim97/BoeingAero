$fn=10;
length = 2;
wingspan = 1.778;

chord = 0.254;
taper = 1;
wing_loc = 0.1;

tail_boom = 0.15;

tail_span = 0.2286;
tail_chord = 0.26;
tail_taper = 0.8571;

vert_span = 0.2413;
vert_chord = 0.2032;
vert_taper = 0.875;

tail_loc = vert_span;

module wing(chord, wingspan, taper, wing_location) {
    color([0.6, 0.6, 0.6]) translate([-0.75*chord, 0]) rotate([90, 0, 180]) linear_extrude(height=wingspan/2, scale=taper) polygon([
        [0, wing_loc],
        [-chord*0.75, wing_loc + 0.05*chord],
        [-chord, wing_loc],
        [-chord*0.75, wing_loc - 0.025*chord]
    ]);
}

module wing_sym(chord, wingspan, taper) {
    color([0.6, 0.6, 0.6]) translate([-0.75*chord, 0]) rotate([90, 0, 180]) linear_extrude(height=wingspan/2, scale=taper) polygon([
        [0, wing_loc],
        [-chord*0.75, wing_loc + 0.05*chord],
        [-chord, wing_loc],
        [-chord*0.75, wing_loc - 0.05*chord]
    ]);
}

module airframe_half() {
    translate([-length*0.35, 0]) wing(chord, wingspan, taper);
    translate([-length*0.7, 0]) wing(tail_chord, tail_span, tail_taper);
    translate([-length*0.7, 0]) rotate([90, 0, 0]) 
        wing(vert_chord, vert_span, vert_taper);
}

module body() {
    color([0.6, 0.6, 0.6]) rotate([0, -90, 0]) rotate_extrude() polygon([
        [0,0],
        [0.015*wingspan,0.01*length],
        [0.04*wingspan,0.075*length],
        [0.055*wingspan,0.15*length],
        [0.06*wingspan,0.25*length],
        [0.065*wingspan,0.3*length],
        [0.0625*wingspan,0.35*length],
        [0.0575*wingspan,0.45*length],
        [0.05*wingspan,0.5*length],
        [0.03*wingspan,0.515*length],
        [0.015*wingspan,0.515*length],
        [0.015*wingspan,0.80*length],
        [0.02*wingspan,0.81*length],
        [0.02*wingspan,0.96*length],
        [0.015*wingspan,0.98*length],
        [0,length]
    ]);
}

module vertical_tail() {
    color([0.6, 0.6, 0.6]) translate([-0.81*length, 0]) rotate([0, 0, 180]) linear_extrude(height=vert_span, scale=vert_taper) polygon([
        [0, 0],
        [0.05*length, 0.025],
        [0.1*length, 0.015],
        [0.15*length, 0]
    ]);
}

module horizontal_tail() {
    color([0.6, 0.6, 0.6]) translate([-0.94*length, 0]) rotate([90, 0, 180]) linear_extrude(height=tail_span, scale=1) polygon([
        [0, tail_loc],
        [-tail_span*0.75, tail_loc + 0.05*tail_chord],
        [-tail_span*1.2, tail_loc],
        [-tail_span*0.75, tail_loc - 0.05*tail_chord]
    ]);
}

module pylon() {
    color([0.6, 0.6, 0.6]) translate([-0.375*length, 0]) rotate([0, 0, 180]) linear_extrude(height=0.2, scale=taper) polygon([
        [0, 0],
        [0.025*length, 0.01],
        [0.05*length, 0.0075],
        [0.075*length, 0]
    ]);
}

module motor() {
    color([0.6, 0.6, 0.6]) translate([-0.35*length, 0]) rotate([0, -90, 0]) rotate_extrude() polygon([
        [0,0],
        [0.015*wingspan,0.01*length],
        [0.02*wingspan,0.015*length],
        [0.025*wingspan,0.025*length],
        [0.025*wingspan,0.045*length],
        [0.025*wingspan,0.065*length],
        [0.0235*wingspan,0.085*length],
        [0.0245*wingspan, 0.11*length],
        [0.025*wingspan, 0.11*length],
        [0.025*wingspan, 0.165*length],
        [0, 0.165*length]
    ]);
}

module propeller() {
    
}

module airframe_no_cuts() {
    body();
    translate([-length*0.35, 0]) wing(chord, wingspan, taper);
    mirror([0, 1, 0]) translate([-length*0.35, 0]) wing(chord, wingspan, taper);
    vertical_tail(1);
    mirror([0, 1, 0]) vertical_tail(1);
    horizontal_tail(1);
    mirror([0, 1, 0]) horizontal_tail(1);
    pylon(1);
    mirror([0, 1, 0]) pylon();
    translate([0, 0, 0.225]) motor(1);
}

module aileron_cut(s) {
    translate([-0.45*length, wingspan*0.4, wing_loc]) scale([s, s, 1]) cube([length*0.1, wingspan*0.15, length*0.1], center=true);
}

module flap_cut(s) {
    translate([-0.45*length, wingspan*0.15, wing_loc]) scale([s, s, 1]) cube([length*0.1, wingspan*0.15, length*0.1], center=true);
}

module elevator_cut(s) {
    translate([-0.915*length, tail_span*0.4, 0.3]) scale([s, s, 1]) cube([length*0.05, wingspan*0.08, length*0.075], center=true);
}

module rudder_cut(s) {
    translate([-length*0.95, 0, length*0.069]) scale([s, s, 1]) cube([length*0.1, wingspan*0.2, length*0.08], center=true);
}

module airframe() {
    difference() {
        airframe_no_cuts();
        aileron_cut(1);
        mirror([0, 1, 0]) aileron_cut(1);
        flap_cut(1);
        mirror([0, 1, 0]) flap_cut(1);
        elevator_cut(1);
        mirror([0, 1, 0]) elevator_cut(1);
        rudder_cut(1);
        mirror([0, 1, 0]) rudder_cut(1);
        translate([-4.5, 0, 0.5]) cube([2, 0.8, 2], center=true); // cockpit
    }
}

module aileron() {
    color("red") render() intersection() {
        aileron_cut(0.99);
        airframe_no_cuts();
    }
}

module flap() {
    color("green") render() intersection() {
        flap_cut(0.99);
        airframe_no_cuts();
    }
}

module elevator() {
    color("blue") render() intersection() {
        elevator_cut(0.99);
        airframe_no_cuts();
    }
}

module rudder() {
    color("black") render() intersection() {
        rudder_cut(0.99);
        airframe_no_cuts();
    }
}

module assembly() {
    airframe();
    aileron();
    mirror([0, 1, 0]) aileron();
    flap();
    mirror([0, 1, 0]) flap();
    rudder();
    elevator();
    mirror([0, 1, 0]) elevator();
}

part = "assembly";

if (part == "airframe") {
    airframe();
} else if (part == "left_aileron") {
    aileron();
} else if (part == "left_flap") {
    flap();
} else if (part == "left_elevator") {
    elevator();
} else if (part == "right_aileron") {
    mirror([0, 1, 0]) aileron();
} else if (part == "right_flap") {
    mirror([0, 1, 0]) flap();    
} else if (part == "right_elevator") {
    mirror([0, 1, 0]) elevator();
} else if (part == "rudder") {
    rudder();
} else {
    assembly();
}
