#!/bin/python3

from xml.dom.minidom import Notation
from matplotlib.widgets import SliderBase
import vedo
from vedo import *
import numpy

mesh = Mesh('xarm-s3.stl')
mesh = Mesh('xarm-s4.stl')
mesh = Mesh('xarm-s5.stl')
mesh = Mesh('xarm-s6.stl')
mesh = Mesh('xarm-sbase.stl')

plt = Plotter(title='xArm')

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

class Segment:
    def __init__(self, parent, axis_of_rotation, direction, rotation, length, mesh, servo_num):
        self.axis_of_rotation = axis_of_rotation
        self.direction = direction
        self.initial_direction = direction
        self.old_direction = direction

        self.parent = parent
        self.length = length
        self.mesh = mesh
        self.servo_num = servo_num

    """
    def current_axis(self): # returns direction vector only, of a sum of each previous segment
        if self.parent is None:
            return self.direction
        
        axis = self.axis_of_rotation

        # cross parent vector and axis of rotation

        cross = np.cross(self.parent.current_axis(), self.parent.initial_direction)
        cross = np.cross(cross, axis)

        rotated_parent = rotate_vector_3d(self.parent.current_axis(), self.axis_of_rotation, 90)

        segment_direction = rotate_vector_3d(self.parent.current_axis(), self.axis_of_rotation, self.rotation * 360)
        segment_direction = rotate_vector_3d(self.parent.current_axis(), cross, self.rotation * 360)

        # cross parent segment_vector and 

        #return rotate_vector_3d(self.parent.sum_axis(), np.cross(self.parent.sum_axis(), self.axis_of_rotation), self.rotation * 360)
        #return segment_direction
        return segment_direction / numpy.linalg.norm(segment_direction) # normalize
     """
    
    """
    def current_axis(self):
        if self.parent is None:
            return self.direction

        parent_axis = self.parent.current_axis()
        transformed_direction = rotate_vector_3d(self.initial_direction, parent_axis, 0)
        #segment_direction = rotate_vector_3d(self.initial_direction, self.axis_of_rotation, self.rotation * 360)
        segment_direction = rotate_vector_3d(transformed_direction, self.axis_of_rotation, self.rotation * 360)

        return segment_direction / numpy.linalg.norm(segment_direction)
    """

    """
    def current_axis(self):
        if self.parent is None:
            return self.direction
        
        parent_axis = self.parent.current_axis()
        segment_direction = rotate_vector_3d(parent_axis, self.axis_of_rotation, self.rotation * 360)
        
        return segment_direction / numpy.linalg.norm(segment_direction)
    """

    
    def current_axis(self):
        if self.parent is None:
            return self.direction

        parent_axis = self.parent.current_axis()
        parent_initial_direction = self.parent.initial_direction

        
        axis_rotation = np.cross(parent_axis, self.initial_direction)
        axis_rotation = axis_rotation / numpy.linalg.norm(axis_rotation)
        #if axis_rotation[0] < 0 or axis_rotation[1] < 0:
            #axis_rotation = axis_rotation * -1
        #axis_rotation = np.fabs(axis_rotation)
        

        if numpy.allclose(axis_rotation, [0,0,0]) == True:
            axis_rotation = self.axis_of_rotation
        if self.servo_num == 6: #Rotating base
            axis_rotation = [0,0,1]
            parent_axis = [0,1,0]
        if self.servo_num == 5: #Servo 5, first y rotation
            #parent_axis = np.cross(rotate_vector_3d(parent_axis, [0,0,1], -90), parent_axis)
            axis_rotation = rotate_vector_3d(parent_axis, [0,0,1], -90)



        segment_direction = rotate_vector_3d(parent_axis, axis_rotation, self.rotation * 360)

        if self.servo_num == 5:
            segment_direction = rotate_vector_3d(segment_direction, [0,0,1], 90)

        if self.servo_num == 3:
            print("segment:", self.servo_num, "axis_rotation:", axis_rotation, "parent_axis:", parent_axis, "initial_direction:", self.initial_direction, "direction:", segment_direction)

        return segment_direction / numpy.linalg.norm(segment_direction)
    

    def get_segment_vector(self):
        return self.current_axis() * self.length

    def get_origin(self): # returns origin position of the current segment
        if self.parent is None:
            return vector(0,0,0)
        if self.parent.length < 0.1:
            return self.parent.get_segment_vector() + self.parent.get_origin() + [0,0,35.0] #add real height instead
        return self.parent.get_segment_vector() + self.parent.get_origin()
    
    def set_mesh_pos(self):  
        self.origin = self.get_origin()          

        direction = self.current_axis() / numpy.linalg.norm(self.current_axis())

        self.mesh.pos(0,0,0)
        if numpy.allclose(self.old_direction, direction) == False:
            self.mesh.reorient(self.old_direction, direction, rad=True) # must set new direction relative to old direction
            print("reorient old_direction: ", self.old_direction, " direction: ", direction, " servo_num", self.servo_num)
        self.mesh.pos(self.origin)

        self.old_direction = direction

    origin = vector(0, 0, 0) # origin of the individual segment
    direction = None # direction normal vector of the individual segment
    old_direction = None
    initial_direction = None
    parent = None
    mesh = None
    rotation = 0
    length = 0
    axis_of_rotation = None

x_axis = vector(1, 0, 0)  # Specify the axis of rotation
y_axis = vector(0, 1, 0)  # Specify the axis of rotation
z_axis = vector(0, 0, 1)  # Specify the axis of rotation
z_direction = z_axis
y_direction = y_axis
x_direction = x_axis

s_base = Segment(None, z_axis, z_direction, 0, 46.0, Mesh('xarm-sbase.stl'), 7) # was 46.0 for length, real length is 65
s_6 = Segment(s_base, z_axis, y_direction, 0, 0.0, Mesh('xarm-s6.stl'), 6) # was 35.0 for length
s_5 = Segment(s_6, y_axis, z_direction, 0, 96.0, Mesh('xarm-s5.stl'), 5)
s_4 = Segment(s_5, y_axis, z_direction, 0, 96.0, Mesh('xarm-s4.stl'), 4)
s_3 = Segment(s_4, y_axis, z_direction, 0, 103.0, Mesh('xarm-s3.stl'), 3)

def slider_base(widget, event):
    s_base.rotation = widget.value

def slider_6(widget, event):
    s_6.rotation = widget.value

def slider_5(widget, event):
    s_5.rotation = widget.value

def slider_4(widget, event):
    s_4.rotation = widget.value

def slider_3(widget, event):
    s_3.rotation = widget.value
    
segments = [s_base, s_6, s_5, s_4, s_3]
sliders = [slider_base, slider_6, slider_5, slider_4, slider_3]

for x in range(1, 5):
    segment = segments[x]
    slider = sliders[x]

    size_x = 0.0
    size_y = 0.2
    offset_x = 0.03
    offset_y = 0.01

    plt.add_slider(
        slider,
        xmin=0.630,
        xmax=1.370,
        value=1,
        c="blue",
        pos=([offset_x * (x + 1), offset_y], [offset_x * (x + 1), size_y + offset_y]),
        title="alpha value (opacity)",
    )
    segment.rotation = 0.0

for s in segments:
    plt += s.mesh

def loop_func(evt):
    for seg in segments:
        seg.set_mesh_pos()
    plt.render()

plt.add_callback("timer", loop_func)
plt.timer_callback("start")
plt.show().close()