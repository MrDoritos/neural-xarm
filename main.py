#!/bin/python3

import os
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = 'hide'
#from xml.dom.minidom import Notation
#from matplotlib.widgets import SliderBase
import vedo
from vedo import *
import numpy
import vedo.addons
import vedo.vtkclasses
import pygame
import importlib

from thirdparty import xarm_control as xarm

plt = Plotter(title='xArm')

x_axis = vector(1, 0, 0)
y_axis = vector(0, 1, 0)
z_axis = vector(0, 0, 1)
z_direction = z_axis
y_direction = y_axis
x_direction = x_axis

"""
Not mine, taken from https://stackoverflow.com/questions/6802577/python-rotation-of-3d-vector
"""
def rotate_vector_3d(vector, axis, angle_degrees):
    """
    Rotate a 3D vector around a specified axis by a given angle.

    Parameters:
        vector: numpy array
            The 3D vector to be rotated.
        axis: numpy array
            The axis of rotation. This should be a 3D unit vector.
        angle_degrees: float
            The angle of rotation in degrees.

    Returns:
        numpy array
            The rotated 3D vector.
    """
    # Convert angle to radians
    angle_radians = np.radians(angle_degrees)
    
    # Rotation matrix formula (Rodrigues' rotation formula)
    rotation_matrix = (
        np.cos(angle_radians) * np.eye(3) +
        (1 - np.cos(angle_radians)) * np.outer(axis, axis) +
        np.sin(angle_radians) * np.array([[0, -axis[2], axis[1]],
                                           [axis[2], 0, -axis[0]],
                                           [-axis[1], axis[0], 0]])
    )
    
    # Perform rotation
    rotated_vector = np.dot(rotation_matrix, vector)
    
    return rotated_vector

def map_to_xy(pos, r=0, a=-z_axis, o=[0,0,0], t=[0,0,0]):
    """
    Map a 3D point on to a 2D plane

    Arguments:
        pos : vector
            point to map
        r : degrees
            rotation of point on axis
        a : vector
            axis of rotation
        o : vector
            origin to remove from pos before map
        t : vector
            translation to add to point after map
    """
    mapped = rotate_vector_3d(pos - o, a, r)
    npos = np.asarray([mapped[0], mapped[2], 0])
    return npos + t

class DirMatrix:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

showDebugObjects = False
updateFunc = None
lastSegment = None
debug_objects = []
perst_objects = []
create_objects = True
use_robot = False
use_joystick = True
elapsed_time = 0
last_frame = 0

class Segment:
    def __init__(self, parent, axis_of_rotation, direction, rotation, length, mesh, servo_num):
        self.axis_of_rotation = axis_of_rotation
        self.direction = direction
        self.initial_direction = direction
        self.old_direction = direction

        self.parent = parent
        self.length = length
        self.mesh = mesh.clone()
        self.source_mesh = mesh.clone()
        self.servo_num = servo_num

    def get_direction_matrix(self):
        base_rotation = np.asarray([x_axis, y_axis, z_axis])

        if self.parent is None:
            return base_rotation
        
        parent_rotation = self.parent.get_direction_matrix()
        segment_rotation = self.rotation * 360
        final_rotation = parent_rotation

        rot_axis_of_rotation = sum([parent_rotation[i] * self.axis_of_rotation[i] for i in range(len(self.axis_of_rotation))])
        rot_axis_of_rotation = rot_axis_of_rotation / numpy.linalg.norm(rot_axis_of_rotation)

        for i in range(0, 3):
            if numpy.allclose(rot_axis_of_rotation, parent_rotation[i]):
                continue

            final_rotation[i] = rotate_vector_3d(parent_rotation[i], rot_axis_of_rotation, segment_rotation)
            final_rotation[i] = final_rotation[i] / numpy.linalg.norm(final_rotation[i])
            
        return final_rotation

    def get_segment_vector(self):
        if self.servo_num == 7:
            return [2.54,0,self.length]
        
        return self.get_direction_matrix()[2] * self.length

    def get_origin(self):
        if self.parent is None:
            return vector(0,0,0)
        
        if self.servo_num == 5:
            return (self.parent.get_segment_vector() + 
                   self.parent.get_origin() + 
                   (rotate_vector_3d(self.get_direction_matrix()[1], z_axis, 90) * -2.54))
        
        return self.parent.get_segment_vector() + self.parent.get_origin()
    
    def get_line(self):
        global create_objects, debug_objects
        
        if not create_objects:
            return

        rotVec = self.dir_matrix
        xVec = Line(self.origin, rotVec[0] * 50 + self.origin, c="red", lw=3)
        yVec = Line(self.origin, rotVec[1] * 50 + self.origin, c="blue", lw=3)
        zVec = Line(self.origin, rotVec[2] * self.length + self.origin, c="green", lw=3)
        debug_objects.append(xVec)
        debug_objects.append(yVec)
        debug_objects.append(zVec)

    def set_mesh_pos(self):  
        self.origin = self.get_origin()
        self.dir_matrix = self.get_direction_matrix()

        dirMatrixChange = bool (numpy.allclose(self.old_dir_matrix, self.dir_matrix, equal_nan=False) == False)

        if dirMatrixChange:
            LT = vedo.LinearTransform()

            rot_y = np.arccos(np.dot(self.dir_matrix[2][2], z_axis[2]))
            rot_z = np.arctan2(self.dir_matrix[1][1], self.dir_matrix[1][0]) + (np.pi/2)
            LT.T.RotateWXYZ(np.rad2deg(rot_z), z_axis)
            LT.T.RotateWXYZ(np.rad2deg(rot_y), np.cross(z_axis, self.dir_matrix[2]))

            #print("rot_y:", np.rad2deg(rot_y), "rot_z:", np.rad2deg(rot_z))
            #print("dir_matrix:", self.dir_matrix)
            #print("dot y:", dot_y, "dot y2:", dot_y2)
            #print("dot z:", dot_z, "dot z2:", dot_z2)
            LT.T.Translate(self.origin)
            tp = vedo.vtkclasses.new("TransformPolyDataFilter")
            tp.SetTransform(LT.T)
            tp.SetInputData(self.source_mesh.dataset)
            tp.Update()
            self.mesh.dataset.DeepCopy(tp.GetOutput())

        self.old_origin = self.origin
        self.old_dir_matrix = self.dir_matrix

    none_vec = vector(0,0,0)
    origin = none_vec # origin of the individual segment
    old_origin = none_vec
    direction = none_vec # direction normal vector of the individual segment
    old_direction = none_vec
    dir_matrix = none_vec
    old_dir_matrix = none_vec
    initial_direction = none_vec
    parent = None
    mesh = None
    angleth = 0
    source_mesh = None
    rotation = 0
    length = 0
    axis_of_rotation = None


s_base = Segment(None, z_axis, z_direction, 0, 46.19, Mesh('assets/xarm-sbase.stl', c='blue', alpha=1.0), 7) # was 46.0 for length, real length is 65
s_6 = Segment(s_base, z_axis, z_direction, 0, 35.98, Mesh('assets/xarm-s6.stl', c='blue', alpha=1.0), 6) # was 35.0 for length
s_5 = Segment(s_6, y_axis, z_direction, 0, 98.0, Mesh('assets/xarm-s5.stl', c='blue', alpha=1.0), 5)
s_4 = Segment(s_5, y_axis, z_direction, 0, 96.0, Mesh('assets/xarm-s4.stl', c='blue', alpha=1.0), 4)
s_3 = Segment(s_4, y_axis, z_direction, 0, 150.0, Mesh('assets/xarm-s3.stl', c='blue', alpha=1.0), 3)

def slider_alpha(widget, event):
    for s in segments:
        s.mesh.alpha(widget.value)

def button_showLines(widget, event):
    widget.switch()

segments = [s_base, s_6, s_5, s_4, s_3]

coords = []
servos = []

plt.add_slider(
    slider_alpha,
    xmin=0.0,
    xmax=1.0,
    value=1,
    c="blue",
    pos=([0.95, 0.03], [0.95, 0.23]),
    title="Alpha",
    show_value=False
)

def get_target_coords():
    return s_3.get_segment_vector() + s_3.get_origin()

coordlim = 500

def get_slider_coords():
    return [x.value * coordlim for x in coords]

showLinesButton = plt.add_button(fnc=button_showLines, pos=(0.85, 0.95), states=["Show Lines", "Hide Lines"], c=["green", "red"])
targetCoordsText = vedo.Text2D("", pos=(0.05, 0.95), s=0.5, c="black")
fpsText = vedo.Text2D("", pos=(0.05, 1), s=0.5, c="black")

axes_box = Box(pos=(0,0,250), length=500, width=500, height=500)

def show_radius(segment):
    global create_objects, debug_objects
    if not create_objects:
        return

    length = 0
    count = False
    for x in segments:
        if x == segment:
            count = True
        if count:
            length += x.length

    origin = segment.get_origin()
    ep = get_target_coords()

    line = vedo.Line(origin, ep, c="blue", lw=3)
    point = vedo.Sphere(pos=origin, c="black", r=segment.length, alpha=0.2)
    cir = vedo.Circle(pos=origin, r=length, c="red", alpha=0.5)
    dir = segment.get_direction_matrix()
    cir.reorient(segment.axis_of_rotation, z_axis)
    cir.reorient(y_axis, dir[1])

    debug_objects.append(line)
    debug_objects.append(point)
    debug_objects.append(cir)

def update_coord_text():
    targetCoordsText.text("Target Coordinates: " + str(get_target_coords()) + "\nSlider Coordinates: " + str(get_slider_coords()) + "\nServo Angles: " + str([x.rotation for x in segments]))

# Set coordinates
def update_target_coords():
    global updateFunc, lastSegment, coordlim
    #print("Updating target coordinates")
    coord = get_target_coords()
    update_coord_text()
    for i in range(len(coords)):
        coords[i].value = coord[i] / coordlim
    if lastSegment:
        show_radius(lastSegment)
    updateFunc = update_target_coords

# Set servo angles
def update_servo_positions():
    global plt, updateFunc, lastSegment, debug_objects, showDebugObjects
    #print("Updating servo positions")
    for servo in servos:
        servo.value = servo.segment.rotation
    update_coord_text()
    show_radius(s_5)
    show_radius(s_6)
    lastSegment = None
    updateFunc = update_servo_positions

updateFunc = update_target_coords

plt += targetCoordsText
plt += fpsText

def slider_rotation(widget, event):
    global lastSegment
    # get slider servos
    widget.segment.rotation = widget.value
    lastSegment = widget.segment

    # update target coordinates
    update_target_coords()

def fast_disc(pos, r, c="red", alpha=1):
    return Disc(pos, r2=r, r1=r-12, c=c, alpha=alpha)

def distance(p1, p2):
    sum = 0
    for i in range(0, min(len(p1), len(p2))):
        sum += (p1[i] - p2[i]) ** 2
    return sqrt(sum)

def move_to_target(widget, event):
    global s_3, s_4, s_5, s_6, s_base, plt, perst_objects, segments, showDebugObjects, create_objects

    bad_calculation = False
    # get slider xyz
    target_coords = get_slider_coords()
    target_2d = np.array([target_coords[0], target_coords[1]])
    debug_objects = perst_objects
    debug_objects.clear()

    # 6 rotation
    seg_6 = s_6.get_origin()
    seg2d_6 = np.array([seg_6[0], seg_6[1]])
    
    dif_6 = target_2d - seg2d_6
    atan2_6 = np.arctan2(dif_6[1], dif_6[0])
    norm_6 = atan2_6 / np.pi # -1 to 1
    rot_6 = (norm_6 + 1) / 2 # 0-1
    deg_6 = rot_6 * 360# + 180 # good for plane 0-360
    serv_6 = rot_6 # 0-2

    """
    norm = atan2 / pi -> -1 to 1
    rot = (norm + 1) / 2 -> 0 to 1
    deg = rot * 360 -> 0 to 360
    serv = rot


    .25 rot = 90 deg
    .37 rot = 133.2 deg
    .630 -> 1.370 = 
    -.370 -> .370 = 
    """

    s_6.rotation = serv_6
    print("atan2_6:", atan2_6, norm_6, "deg_6:", deg_6, "rot_6:", rot_6, "serv_6:", serv_6, serv_6 * 360)

    seg_5 = s_5.get_origin()

    # target point must be forward of servo 5. deg_6 is the rotation of servo 6 which completes this
    # we can successfully ignore 3d state beyond this point, all calculations can be done in 2d
    target_pl3d = map_to_xy(target_coords, deg_6, o=seg_5, a=-z_axis)
    target_pl2d = target_pl3d[0:2]
    plo3d = [0,0,0]
    plo2d = plo3d[0:2]
    pl3d = np.asarray([500,0,0])
    pl2d = pl3d[0:2]
    s2d = np.asarray([500,500])

    # rotated point in world
    if create_objects:
        debug_objects.append(Point(target_coords, c="black"))
        debug_objects.append(Arrow(seg_5, target_coords))
        debug_objects.append(Arrow(seg2d_6, target_2d))
        debug_objects.append(Plane(pl3d-[0,0,0.1], s=s2d))
        debug_objects.append(Point(pl3d))
        debug_objects.append(Point(target_pl3d + pl3d, c="black"))

    remaining_segments = segments[2:]
    remaining_segments.reverse()
    prev_origin = target_pl2d
    prev_length = s_3.length
    total_distance = 0
    new_origins = []

    while True:
        if not remaining_segments or len(remaining_segments) < 1:
            if showDebugObjects:
                print("no more segments")
            break
        
        seg = remaining_segments[0]
        segment_radius = seg.length
        total_length = sum([x.length for x in remaining_segments])
        dist_to_segment = total_length - segment_radius
        segment_min = total_length - (segment_radius * 2)
        dist_origin_to_prev = distance(plo3d, prev_origin)
        mag = prev_origin - plo2d
        mag = mag / np.linalg.norm(mag)

        skip_optim = False
        color = "green"

        if len(remaining_segments) < 3:
            if showDebugObjects:
                print("2 or less segments left")
            skip_optim = True
        
        equal_mp = ((dist_origin_to_prev ** 2) - (segment_radius ** 2) + (dist_to_segment ** 2)) \
                        / (2 * dist_origin_to_prev)
        rem_dist = dist_origin_to_prev - equal_mp

        rem_min = -segment_radius/2
        rem_retract = 0
        rem_extend = segment_radius*0.5#*0.75
        rem_ex2 = rem_extend * 1.75
        rem_max = segment_radius*.95

        if rem_dist > rem_max:
            if showDebugObjects:
                print("not enough overlap")
                print("rem_dist:", rem_dist, "rem_max:", rem_max)
            color = "black"
            #rem_dist = rem_max

        if not skip_optim:
            # rem_dist -> -segment_radius - furthest from origin
            # rem_dist -> segment_radius - closest to origin

            # we are very close to center
            if segment_radius > dist_origin_to_prev: # rem_dist < rem_retract and
                if showDebugObjects:
                    print("close to origin point allow all orientations to solve")
                color = "red"
                v = rem_extend - (segment_radius - dist_origin_to_prev)
                equal_mp = dist_origin_to_prev - v#rem_dist
            
            # high left over length and prevent all solutions
            elif rem_dist < rem_extend and total_length > dist_origin_to_prev:
                if showDebugObjects:
                    print("maintain good center of gravity")
                color = "white"
                equal_mp = dist_origin_to_prev - rem_extend

            # too much leftover length, limit pos to radius
            elif rem_dist < rem_retract and total_length > dist_origin_to_prev:
                if showDebugObjects:
                    print("too much leftover length, allow more orientations to reduce length")
                color = "orange"
                equal_mp = dist_origin_to_prev - rem_retract

            elif rem_dist < rem_ex2 and rem_dist >= rem_extend and total_length > dist_origin_to_prev:
                if showDebugObjects:
                    print("too much leftover length, limit pos to radius")
                color = "yellow"
                r = rem_ex2 - rem_extend # 0 -> (rem_ex2 - rem_extend) (0 -> .75 radius)
                r = (rem_dist - rem_extend) / r
                v = r / 2
                # 0 -> white
                # 0.5 -> green
                if (v > 0.4):
                    v -= (v - 0.38)
                equal_mp = dist_origin_to_prev - (v * segment_radius + rem_extend)


            if rem_dist < rem_min:
                if showDebugObjects:
                    print("too much overlap")
                color = "gray"
                rem_dist = rem_min
            elif rem_dist > rem_max:
                if showDebugObjects:
                    print("not enough overlap")
                color = "black"
            else:
                rem_dist = dist_origin_to_prev - equal_mp


            debug_objects.append(Text2D(str.format("rem_dist: {0:.2f}, equal_mp: {0:.2f}", rem_dist, equal_mp), pos=(0.5, 0.5), s=0.5, c="black"))

        # vector for mp calc
        mp_vec = mag * equal_mp
        n = np.sqrt((segment_radius ** 2) - (rem_dist ** 2))
        o = np.arctan2(mag[1], mag[0]) - (np.pi / 2)
        new_mag = np.asarray([np.cos(o), np.sin(o)])
        new_origin = mp_vec + (new_mag * n)
        new_origin3d = [new_origin[0], new_origin[1], 0]
        dist_new_prev = distance(new_origin, prev_origin)

        if bad_calculation:
            color = "red"

        tolerable_distance = 10

        if abs(dist_new_prev - segment_radius) > tolerable_distance:
            if showDebugObjects:
                    print("distance to prev is too different")
            color = "purple"
            bad_calculation = True

        if len(remaining_segments) < 1 and distance(new_origin, target_pl2d) > tolerable_distance:
            if showDebugObjects:
                    print("distance to target is too far")
            color = "cyan"
            bad_calculation = True



        if create_objects:
            debug_objects.append(Point(mp_vec + pl2d, c="blue"))
            debug_objects.append(fast_disc(new_origin + pl2d, segment_radius, c="green"))
            #debug_objects.append(Triangle(new_origin + pl2d, prev_origin + pl2d, mp_vec + pl2d, c="green"))
            debug_objects.append(Point(new_origin + pl2d, c="black"))
            debug_objects.append(Arrow2D(new_origin + pl2d, prev_origin + pl2d, c=color))
            debug_objects.append(vedo.addons.Flagpost(str(seg.servo_num), new_origin + pl2d, new_origin3d + pl3d + [0,0,1]))

        if showDebugObjects:
            print("segment:", seg.servo_num,
                "segment_radius:", segment_radius,
                "dist_to_segment:", dist_to_segment,
                "dist_origin_to_prev:", dist_origin_to_prev,
                "total_length:", total_length,
                "segment_min:", segment_min,
                "equal_mp:", equal_mp,
                "rem_dist:", rem_dist,
                "mag:", mag,
                "mp_vec:", mp_vec, 
                "new_mag:", new_mag,
                "n:", n, 
                "o:", np.rad2deg(o), 
                "r:", np.rad2deg(np.arccos(np.dot(new_mag, mag))),
                "new_origin:", new_origin,
                "prev_origin:", prev_origin,
                "dist_new_prev:", dist_new_prev)
            

        prev_origin = new_origin
        new_origins.append(new_origin)
        remaining_segments.pop(0)
        
        
    if bad_calculation:
        if showDebugObjects:
            print("failed to calculate")
    else:
        prevrot = 0
        prevmag = [0,1]
        prev = [0,0]
        new_origins.reverse()
        new_origins.pop(0)
        new_origins.append(target_pl2d)
        for i in range(len(new_origins)):
            cur = new_origins[i]

            dif = cur - prev
            mag = dif / np.linalg.norm(dif)
            
            servo = segments[i + 2]
            calcmag = mag
            
            rot = np.arctan2(calcmag[0], calcmag[1]) - prevrot
            
            deg = ((np.rad2deg(rot)) / 180) * 0.5 + 1
            servo.rotation = deg

            if showDebugObjects:
                print("servo:", servo.servo_num, "dt:", np.linalg.det([[0,0],mag]), "dp:", np.dot(prevmag, mag),"calcmag:", calcmag, "diffmag:", mag-prevmag, "prevmag:", prevmag, "mag:", mag, "prevrot:", np.rad2deg(prevrot), "rot:", np.rad2deg(rot), "deg:", deg * 360, deg, "cur:", cur, "prev:", prev)
                
            prev = cur
            prevmag = mag
            prevrot = prevrot + rot

    update_servo_positions()

for x in range(0, 3):
    i = "XYZ"[x]
    x = x+1

    size_x = 0.06
    size_y = 0.2
    offset_x = 0.03
    offset_y = 0.3

    _slider = Slider2D(
        move_to_target,
        xmin=-1.0,
        xmax=1.0,
        value=1,
        c="blue",
        pos=([offset_x + (x * size_x) - size_x, offset_y], [offset_x + (x * size_x) - size_x, size_y + offset_y]),
        title=i,
        show_value=False
    )

    plt.sliders.append([_slider, move_to_target])
    coords.append(_slider)
    _slider.interactor = plt.interactor
    _slider.on()

for x in range(1, 5):
    segment = segments[x]

    size_x = 0.06
    size_y = 0.2
    offset_x = 0.03
    offset_y = 0.01

    _slider = Slider2D(
        slider_rotation,
        xmin=0.630,
        xmax=1.370,
        value=1,
        c="blue",
        pos=([offset_x + (x * size_x) - size_x, offset_y], [offset_x + (x * size_x) - size_x, size_y + offset_y]),
        title="Servo " + str(segment.servo_num),
        show_value=False
    )
    _slider.segment = segment
    segment.rotation = 0.0
    plt.sliders.append([_slider, slider_rotation])
    servos.append(_slider)
    _slider.interactor = plt.interactor
    _slider.on()

for s in segments:
    plt += s.mesh

joysticks = []
clock = pygame.time.Clock()

map_axis = {
    0: 1,
    1: 0,
    4: 2
}
wrist = 0
gripper = 0
last_wg = [0,0]
mirror_axis = [2]
last_axis_values = [0,0,0]
last_move_values = [0,0,0,0,0,0]
robot = None
last_send = 0
send_interval = 50

def handle_chrono():
    global last_frame, elapsed_time

    now = pygame.time.get_ticks()
    elapsed_time = now - last_frame
    last_frame = now

    fpsText.text(str(1000 / elapsed_time) + " FPS")

def handle_input():
    global joysticks, clock, coords, last_axis_values, robot, last_send, gripper, wrist
    global elapsed_time

    if not use_joystick:
        return

    change = False
    clock.tick(elapsed_time)
    for event in pygame.event.get():
        if 'device_index' in event.dict and 'guid' in event.dict:
            if any([event.guid == x.get_guid() for x in joysticks]):
                continue

            j = pygame.joystick.Joystick(event.device_index)
            j.init()
            joysticks.append(j)

        if 'joy' in event.dict and 'axis' in event.dict and event.axis in [2,5,3]:
            if event.axis == 2:
                #gripper += event.value * 0.01
                last_wg[0] = -(event.value + 1)
            elif event.axis == 5:
                #gripper -= event.value * 0.01
                last_wg[0] = (event.value + 1)
            elif event.axis == 3:
                #wrist += event.value * 0.01
                last_wg[1] = event.value
                

        if 'joy' in event.dict and 'axis' in event.dict and event.axis in map_axis:
            map = map_axis[event.axis]
            val = event.value

            if map in mirror_axis:
                val = -val
            
            if (abs(val) < 0.2):
                val = 0 # deadzone

            last_axis_values[map] = val
    
    if abs(last_wg[0]) > 0.1:
        gripper += last_wg[0] * 0.05
        if (abs(gripper) > 1):
            gripper /= abs(gripper)
        
    if abs(last_wg[1]) > 0.2:
        wrist += last_wg[1] * 0.05
        if (abs(wrist) > 1):
            wrist /= abs(wrist)

    for i in range(0, 3):
        adval = last_axis_values[i] * 0.01
        coords[i].value += adval

        #rcoords = get_slider_coords()

        #if (s_5.get_origin()[i] + rcoords[i]) > 250:
        #    coords[i].value = 250 / coordlim

        if abs(adval) > 0:
            change = True

            if abs(coords[i].value) > 1:
                coords[i].value /= abs(coords[i].value)

    if change:
        move_to_target(None, None)

def handle_output():
    global last_send, last_move_values

    if not use_robot:
        return
    
    if last_send + send_interval < pygame.time.get_ticks():
        #robot.move_to(get_target_coords())
        axis = [0, 0, 0, 0, wrist, gripper]
        flip = [1, 1, -1, 1, 1, 1]
        elapsed = pygame.time.get_ticks() - last_send
        if elapsed > 1000:
            elapsed = 1000
        move_per_second = 100
        norm_per_second = move_per_second / (100 * 1)
        max_move = (elapsed / 1000) * norm_per_second

        for i in range(0, 4):
            val = segments[i + 1].rotation * flip[i]
            
            if not np.isfinite(val):
                return

            mult = .450 / .250
            # val just means 360 degrees per 1
            deg = val * 360 + 180
            mod = np.mod(deg, 360)
            if mod < 0:
                mod += 360

            axis_val = ((mod / 180) - 1) * mult
            
            if abs(axis_val) > 1.25 and i == 0:
                axis_val = (axis_val / abs(axis_val)) * 1.25

            diff = axis_val - last_move_values[i]
            dist = sqrt(diff ** 2)
            if dist > max_move and i == 0:
                new_val = last_move_values[i] + (np.sign(diff) * max_move)
                print("clamp axis:", i, "axis_val:", axis_val, "last_move_value", last_move_values[i], "new_val:", new_val, "max_move:", max_move, "diff:", diff, "dist:", dist)
                axis_val = new_val

            axis[i] = axis_val

        if all(np.isfinite(axis)) and use_robot and not np.allclose(axis, last_move_values):
            print("sending:", axis)
            robot.move_all(axis, send_interval)
            last_move_values = axis
            last_send = pygame.time.get_ticks()

def loop_func(evt):
    global plt, showLinesButton, updateFunc, showDebugObjects, debug_objects, perst_objects

    plt.remove("line")
    debug_objects = []
    showDebugObjects = showLinesButton.status_idx

    handle_chrono()

    handle_input()

    handle_output()

    for seg in segments:
        seg.set_mesh_pos()
        seg.get_line()
        
    updateFunc()

    if showDebugObjects:
        for x in debug_objects + perst_objects:
            x.name = "line"

        plt += debug_objects
        plt += perst_objects

    plt.render()

def init():
    global robot
    if use_robot:
        robot = xarm.SafeXArm()
        robot.move_all(last_move_values, 2000)

    if use_joystick:
        pygame.init()
        for i in range(0, pygame.joystick.get_count()):
            joysticks.append(pygame.joystick.Joystick(i))
            joysticks[-1].init()
            print ("Detected joystick "),joysticks[-1].get_name(),"'"

def run():
    init()

    plt.add_callback("timer", loop_func)
    plt.timer_callback("start", dt=1)
    #plt.show(axes=3).close()
    plt.show(axes=Axes(axes_box), size=(700,700)).close()

    if robot and use_robot:
        robot.rest()

run()