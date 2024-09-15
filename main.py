#!/bin/python3

from xml.dom.minidom import Notation
from matplotlib.widgets import SliderBase
import vedo
from vedo import *
import numpy
import vedo.addons
import vedo.vtkclasses

plt = Plotter(title='xArm')

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

x_axis = vector(1, 0, 0)
y_axis = vector(0, 1, 0)
z_axis = vector(0, 0, 1)
z_direction = z_axis
y_direction = y_axis
x_direction = x_axis

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
        base_rotation = [x_axis, y_axis, z_axis]

        if self.parent is None:
            return base_rotation
        
        parent_rotation = self.parent.get_direction_matrix()
        rotation = base_rotation
        segment_rotation = self.rotation * 360
        

        final_rotation = parent_rotation
        trans_axis_of_rotation = self.axis_of_rotation

        for i in range(0, 3):
            if self.axis_of_rotation[i] != 0:
                trans_axis_of_rotation = parent_rotation[i]

        for i in range(0, 3):
            if numpy.allclose(trans_axis_of_rotation, parent_rotation[i]):
                continue

            final_rotation[i] = rotate_vector_3d(parent_rotation[i], trans_axis_of_rotation, segment_rotation)
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
                   (rotate_vector_3d(self.get_direction_matrix()[1], z_axis, 90) * 2.54))
        
        return self.parent.get_segment_vector() + self.parent.get_origin()
    
    def get_line(self):
        output = []
        
        #initialLine = Line(self.origin, self.initial_direction * self.length + self.origin, c="red")
        #rotationLine = Line(self.origin, self.axis_of_rotation * self.length * 0.5 + self.origin, c="blue")
        #angleLine = Box(pos=self.origin, size=(50,100,50), c="pink", alpha=0.5)
        #angleLine.rotate(axis=self.axis_of_rotation, angle=self.angleth, rad=True)
        #upLine = Line(self.origin, self.up_axis() * 50 * 2.5 + self.origin, c="pink")
        #cross = np.cross(self.initial_direction, self.direction)
        #angleLine.reorient(self.initial_direction, cross, rotation=-self.angleth, rad=True)


        #dirLine = Line(self.origin, self.current_axis() * self.length * 2 + self.origin, c="black")
        #crossLine = Line(self.origin, cross * self.length + self.origin, c="green")
        #dirPlane = Plane(self.origin, self.direction, s=(50,50), alpha=0.5, c="gray")
        
        rotVec = self.dir_matrix
        xVec = Line(self.origin, rotVec[0] * 50 + self.origin, c="red", lw=3)
        yVec = Line(self.origin, rotVec[1] * 50 + self.origin, c="blue", lw=3)
        zVec = Line(self.origin, rotVec[2] * self.length + self.origin, c="green", lw=3)
        output.append(xVec)
        output.append(yVec)
        output.append(zVec)
        
        #output.append(initialLine)
        #output.append(rotationLine)
        #output.append(angleLine)
        #output.append(upLine)
        #output.append(dirLine)
        #output.append(crossLine)
        #output.append(dirPlane)


        for x in output:
            x.name = "line"

        return output

    def set_mesh_pos(self):  
        self.origin = self.get_origin()
        self.dir_matrix = self.get_direction_matrix()

        dirMatrixChange = bool (numpy.allclose(self.old_dir_matrix, self.dir_matrix, equal_nan=False) == False)

        if dirMatrixChange:
            LT = vedo.LinearTransform()
            rot_x = np.arccos(np.dot(self.dir_matrix[0], x_axis))
            rot_y = np.arccos(np.dot(self.dir_matrix[2], z_axis))
            #rot_z = np.arccos(np.dot(self.dir_matrix[1], y_axis))
            #rot_z = np.arccos(self.dir_matrix[1][0]) + np.arcsin(self.dir_matrix[1][1])
            x = self.dir_matrix[1][0]
            y = self.dir_matrix[1][1]
            p = np.sqrt(x**2 + y**2)
            rot_z = np.arccos(x / p)

            dot_y = np.dot(self.dir_matrix[2], x_axis)
            dot_y2 = np.dot(self.dir_matrix[2], z_axis)
            dot_z = np.dot(self.dir_matrix[1], x_axis)
            dot_z2 = np.dot(self.dir_matrix[1], y_axis)
            flipY = np.dot(self.dir_matrix[0], z_axis) < 0
            flipZ = dot_z2 < 0

            rot_z = np.arccos(dot_z) + (np.pi/2)

            if np.dot(self.dir_matrix[0], z_axis) < 0:
                #rot_y = -rot_y
                rot_y = ((2*np.pi)-rot_y)
                #LT.T.RotateWXYZ(np.pi, y_axis)
                #LT.T.RotateWXYZ(np.rad2deg(rot_y), y_axis)
            else:
                #LT.T.RotateWXYZ(np.rad2deg(rot_y), y_axis)
                pass
            #LT.T.RotateWXYZ(-self.rotation * 360, y_axis)
            LT.T.RotateWXYZ(np.rad2deg(rot_y), y_axis)


            if dot_z2 < 0:
                rot_z = (np.pi-rot_z)
                LT.T.RotateWXYZ(np.pi, z_axis)
                LT.T.RotateWXYZ(np.rad2deg(rot_z), z_axis)
            else:
                LT.T.RotateWXYZ(np.rad2deg(rot_z), z_axis)

            #print("rot_x:", np.rad2deg(rot_x), "rot_y:", np.rad2deg(rot_y), "rot_z:", np.rad2deg(rot_z))
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


s_base = Segment(None, z_axis, z_direction, 0, 46.19, Mesh('xarm-sbase.stl', c='blue', alpha=1.0), 7) # was 46.0 for length, real length is 65
s_6 = Segment(s_base, z_axis, z_direction, 0, 35.98, Mesh('xarm-s6.stl', c='blue', alpha=1.0), 6) # was 35.0 for length
s_5 = Segment(s_6, y_axis, z_direction, 0, 96.0, Mesh('xarm-s5.stl', c='blue', alpha=1.0), 5)
s_4 = Segment(s_5, y_axis, z_direction, 0, 96.0, Mesh('xarm-s4.stl', c='blue', alpha=1.0), 4)
s_3 = Segment(s_4, y_axis, z_direction, 0, 103.0, Mesh('xarm-s3.stl', c='blue', alpha=1.0), 3)

def slider_alpha(widget, event):
    for s in segments:
        s.mesh.alpha(widget.value)

def slider_rotation(widget, event):
    widget.segment.rotation = widget.value

def button_showLines(widget, event):
    widget.switch()

segments = [s_base, s_6, s_5, s_4, s_3]

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

showLinesButton = plt.add_button(fnc=button_showLines, pos=(0.95, 0.95), states=["Show Lines", "Hide Lines"], c=["green", "red"])


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
    _slider.interactor = plt.interactor
    _slider.on()

for s in segments:
    plt += s.mesh

def loop_func(evt):
    global plt, showLinesButton
    plt.remove("line")
    for seg in segments:
        seg.set_mesh_pos()
        if showLinesButton.status_idx:
            plt.add(seg.get_line())
    plt.render()

plt.add_callback("timer", loop_func)
plt.timer_callback("start", dt=1)
#plt.show(axes=3).close()
plt.show().close()