#!/bin/python3

from xml.dom.minidom import Notation
from matplotlib.widgets import SliderBase
import vedo
from vedo import *
import numpy
import vedo.vtkclasses

#mesh = Mesh('xarm-s3.stl')
#mesh = Mesh('xarm-s4.stl')
#mesh = Mesh('xarm-s5.stl')
#mesh = Mesh('xarm-s6.stl')
#mesh = Mesh('xarm-sbase.stl')

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

x_axis = vector(1, 0, 0)  # Specify the axis of rotation
y_axis = vector(0, 1, 0)  # Specify the axis of rotation
z_axis = vector(0, 0, 1)  # Specify the axis of rotation
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

    def up_axis(self):
        if self.parent is None:
            x = np.cross(self.direction, y_axis)
            return x / numpy.linalg.norm(x)

        parent_axis = self.parent.current_axis()
        parent_up = self.parent.up_axis()
        segment_rotation = self.rotation * 360

        if numpy.allclose(parent_axis, [0,0,0], equal_nan=True, atol=0.01) == True:
            return self.axis_of_rotation

        if self.axis_of_rotation[2] != 0:
            x = rotate_vector_3d(parent_up, parent_axis, segment_rotation)
            return x / numpy.linalg.norm(x)

        if self.axis_of_rotation[1] != 0:
            #axis_rotation = np.cross(self.initial_direction, parent_up)
            axis_rotation = rotate_vector_3d(parent_up, parent_axis, -90) #turns into x axis
            #axis_rotation = axis_rotation / numpy.linalg.norm(axis_rotation)
            #if np.dot(self.initial_direction, parent_up) < 0:
                #segment_rotation = -segment_rotation
            res = rotate_vector_3d(parent_up, axis_rotation, segment_rotation)
            return res / numpy.linalg.norm(res)

        return self.parent.up_axis()


        #axis_rotation = np.cross(z_axis, parent_up)
        #axis_rotation = axis_rotation / numpy.linalg.norm(axis_rotation)

        #if numpy.allclose(axis_rotation, [0,0,0], equal_nan=True, atol=0.01) == True:
        #    axis_rotation = self.axis_of_rotation


        #return rotate_vector_3d(parent_up, axis_rotation, segment_rotation)
    
    def current_axis(self):
        if self.parent is None:
            return self.direction

        parent_axis = self.parent.current_axis()
        parent_up = self.parent.up_axis()
        parent_initial_direction = self.parent.initial_direction
        parent_axis_of_rotation = self.parent.axis_of_rotation

        # flips
        #axis_rotation = np.cross(parent_axis, self.initial_direction)
        axis_rotation = np.cross(self.initial_direction, parent_up)
        #axis_rotation_rot = np.arccos(np.dot())
        axis_rotation = axis_rotation / numpy.linalg.norm(axis_rotation)
        
        segment_rotation = self.rotation * 360

        #print("servo_num:", self.servo_num, "axis_rotation:", axis_rotation, "parent_axis:", parent_axis, "initial_direction:", self.initial_direction, "direction:", self.direction, "up:", parent_up)
        #if numpy.dot(axis_rotation, self.axis_of_rotation) < 0:
        #    axis_rotation = axis_rotation * -1
        #if axis_rotation[0] < 0 or axis_rotation[1] < 0 or axis_rotation[2] < 0:
        #    axis_rotation = axis_rotation * -1
        #axis_rotation = np.fabs(axis_rotation)
        

        if numpy.allclose(axis_rotation, [0,0,0], equal_nan=True, atol=0.01) == True:
            axis_rotation = self.axis_of_rotation
        if self.servo_num == 6: #Rotating base
            axis_rotation = [0,0,1]
            parent_axis = [0,1,0]
        if self.servo_num == 5: #Servo 5, first y rotation
            #parent_axis = np.cross(rotate_vector_3d(parent_axis, [0,0,1], -90), parent_axis)
            axis_rotation = rotate_vector_3d(parent_axis, [0,0,1], -90) # Left of axis
            segment_rotation += 90


        axis_rot_y = rotate_vector_3d(parent_axis, z_axis, 90)
        axis_rot_y[2] = 0
        axis_rot_y = axis_rot_y / numpy.linalg.norm(axis_rot_y)

        #append our direction to the parent axis on the axis of rotation
        our_direction = rotate_vector_3d(self.initial_direction, self.axis_of_rotation, segment_rotation)

        

        segment_direction = rotate_vector_3d(parent_axis, axis_rotation, segment_rotation)
        #segment_direction = rotate_vector_3d(parent_axis, axis_rot_y, segment_rotation)
        #segment_direction = parent_axis + our_direction

        if self.servo_num == 5:
            segment_direction = rotate_vector_3d(segment_direction, [0,0,1], 90)

        #if self.servo_num == 3:
        #    print("segment:", self.servo_num, "axis_rotation:", axis_rotation, "parent_axis:", parent_axis, "initial_direction:", self.initial_direction, "direction:", segment_direction)

        return segment_direction / numpy.linalg.norm(segment_direction)
    

    def get_segment_vector(self):
        #return self.current_axis() * self.length
        return self.get_direction_matrix()[2] * self.length

    def get_origin(self): # returns origin position of the current segment
        if self.parent is None:
            return vector(0,0,0)
        if self.parent.length < 0.1:
            return self.parent.get_segment_vector() + self.parent.get_origin() + [0,0,35.0] #add real height instead
        return self.parent.get_segment_vector() + self.parent.get_origin()
    
    def get_line(self):
        output = []
        
        initialLine = Line(self.origin, self.initial_direction * self.length + self.origin, c="red")
        rotationLine = Line(self.origin, self.axis_of_rotation * self.length * 0.5 + self.origin, c="blue")
        angleLine = Box(pos=self.origin, size=(50,100,50), c="pink", alpha=0.5)
        #angleLine.rotate(axis=self.axis_of_rotation, angle=self.angleth, rad=True)
        upLine = Line(self.origin, self.up_axis() * 50 * 2.5 + self.origin, c="pink")
        cross = np.cross(self.initial_direction, self.direction)
        #angleLine.reorient(self.initial_direction, cross, rotation=-self.angleth, rad=True)


        dirLine = Line(self.origin, self.current_axis() * self.length * 2 + self.origin, c="black")
        crossLine = Line(self.origin, cross * self.length + self.origin, c="green")
        dirPlane = Plane(self.origin, self.direction, s=(50,50), alpha=0.5, c="gray")
        
        rotVec = self.dir_matrix
        xVec = Line(self.origin, rotVec[0] * 50 + self.origin, c="red", lw=3)
        yVec = Line(self.origin, rotVec[1] * 50 + self.origin, c="blue", lw=3)
        zVec = Line(self.origin, rotVec[2] * 50 + self.origin, c="green", lw=3)
        output.append(xVec)
        output.append(yVec)
        output.append(zVec)
        
        output.append(initialLine)
        output.append(rotationLine)
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
        self.direction = self.current_axis() / numpy.linalg.norm(self.current_axis())
        self.dir_matrix = self.get_direction_matrix()

        #self.mesh.dataset = self.source_mesh.dataset
        #self.mesh.dataset = None
        #self.mesh = self.source_mesh.clone()

        originChange = False
        directionChange = False
        dirMatrixChange = False

        if self.old_origin is not None:
            originChange = bool (numpy.allclose(self.old_origin, self.origin, equal_nan=False) == False)
        if self.old_direction is not None:
            directionChange = bool (numpy.allclose(self.old_direction, self.direction, equal_nan=False) == False)
        if self.old_dir_matrix is not None:
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

            print("rot_x:", np.rad2deg(rot_x), "rot_y:", np.rad2deg(rot_y), "rot_z:", np.rad2deg(rot_z))
            print("dir_matrix:", self.dir_matrix)
            print("dot y:", dot_y, "dot y2:", dot_y2)
            print("dot z:", dot_z, "dot z2:", dot_z2)
            LT.T.Translate(self.origin)
            tp = vedo.vtkclasses.new("TransformPolyDataFilter")
            tp.SetTransform(LT.T)
            tp.SetInputData(self.source_mesh.dataset)
            tp.Update()
            self.mesh.dataset.DeepCopy(tp.GetOutput())

        if False and (originChange or directionChange):
        #LT.reorient(self.initial_direction, direction, rad=True)
            LT = vedo.LinearTransform()
            crossdir = np.cross(self.initial_direction, self.direction)
            dotdir = np.dot(self.initial_direction, self.direction)
            angleth = np.arccos(dotdir)
            #LT.T.RotateWXYZ(np.rad2deg(angleth), crossvec)
            
            upcrossvec = np.cross(self.direction, z_direction)

            #global axis
            xglb = x_axis
            yglb = y_axis
            zglb = z_axis

            #relative axis
            yaxis = rotate_vector_3d(self.direction, zglb, 90)
            yaxis[2] = 0
            yaxis = yaxis / numpy.linalg.norm(yaxis)
            xaxis = rotate_vector_3d(yaxis, zglb, 90)
            zaxis = rotate_vector_3d(self.direction, yaxis, -90)

            anglex = 0
            angley = angleth
            anglez = np.arccos(np.dot(yaxis, yglb))
            if np.dot(yaxis, xglb) > 0:
                anglez = -anglez

            anglex = np.rad2deg(anglex)
            angley = np.rad2deg(angley)
            anglez = np.rad2deg(anglez)

            LT.T.RotateWXYZ(angley, yglb)
            LT.T.RotateWXYZ(anglez, zglb)

            self.angleth = angleth
            LT.T.Translate(self.origin)
            print("reorient initial_direction:", self.initial_direction, "old_direction:", self.old_direction, "direction:", self.direction, "servo_num", self.servo_num)
            print("rotation:", self.rotation, "axis_of_rotation:", self.axis_of_rotation)
            print("crossvec:", crossdir, "angleth:", np.rad2deg(angleth))
            print("angle x:", anglex, "angle y:", angley, "angle z:", anglez)
            print("yaxis:", yaxis, "xaxis:", xaxis, "zaxis:", zaxis)
            print("axis_of_rotation:", self.axis_of_rotation, "upcrossvec:", upcrossvec)
            tp = vedo.vtkclasses.new("TransformPolyDataFilter")
            tp.SetTransform(LT.T)
            tp.SetInputData(self.source_mesh.dataset)
            tp.Update()
            self.mesh.dataset.DeepCopy(tp.GetOutput())

        #self.mesh.transform = LT
        #self.mesh.apply_transform(LT, concatenate=False)

        #self.mesh.point_locator = None
        #self.mesh.cell_locator = None
        #self.mesh.line_locator = None
        

        #self.mesh.pos(0,0,0)
        #if numpy.allclose(self.old_direction, direction) == False:
        #    self.mesh.reorient(self.old_direction, direction, rad=True) # must set new direction relative to old direction
        #    print("reorient old_direction: ", self.old_direction, " direction: ", direction, " servo_num", self.servo_num)
        #self.mesh.pos(self.origin)

        self.old_direction = self.direction
        self.old_origin = self.origin
        self.old_dir_matrix = self.dir_matrix

    origin = vector(0,0,0) # origin of the individual segment
    old_origin = vector(0,0,0)
    direction = None # direction normal vector of the individual segment
    old_direction = None
    dir_matrix = None
    old_dir_matrix = None
    initial_direction = None
    parent = None
    mesh = None
    angleth = 0
    source_mesh = None
    rotation = 0
    length = 0
    axis_of_rotation = None


s_base = Segment(None, z_axis, z_direction, 0, 46.0, Mesh('xarm-sbase.stl', c='blue', alpha=0.5), 7) # was 46.0 for length, real length is 65
s_6 = Segment(s_base, z_axis, y_direction, 0, 0.0, Mesh('xarm-s6.stl', c='blue', alpha=0.5), 6) # was 35.0 for length
s_5 = Segment(s_6, y_axis, z_direction, 0, 96.0, Mesh('xarm-s5.stl', c='blue', alpha=0.5), 5)
s_4 = Segment(s_5, y_axis, z_direction, 0, 96.0, Mesh('xarm-s4.stl', c='blue', alpha=0.5), 4)
s_3 = Segment(s_4, y_axis, z_direction, 0, 103.0, Mesh('xarm-s3.stl', c='blue', alpha=0.5), 3)

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
    global plt
    plt.remove("line")
    for seg in segments:
        #plt.remove(seg.mesh)
        seg.set_mesh_pos()
        plt.add(seg.get_line())
        #plt += seg.mesh
    plt.render()

plt.add_callback("timer", loop_func)
plt.timer_callback("start", dt=10)
plt.show(axes=3).close()