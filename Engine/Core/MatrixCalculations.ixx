module;
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>              // Include glm
#include <glm/gtx/transform.hpp>    // glm transform functions.
#include <glm/gtc/quaternion.hpp>// Include quaternions
#include <glm/gtx/quaternion.hpp>// Include quaternions

export module EncosyEngine.MatrixCalculations;

import Components.TransformComponent;

export namespace MatrixCalculations
{
    glm::mat4 CalculateViewMatrix(glm::vec3 position, glm::vec3 eulerAngles, glm::vec3 scale) {
        return glm::inverse(
            glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(eulerAngles.x, glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::rotate(eulerAngles.y, glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::rotate(eulerAngles.z, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), scale));
    }

    glm::mat4 CalculateViewMatrix(glm::vec3 position, glm::quat orientation, glm::vec3 scale) {
        return glm::inverse(
            glm::translate(glm::mat4(1.0f), position)
            * glm::mat4_cast(orientation)
            * glm::scale(glm::mat4(1.0f), scale));
    }

    glm::mat4 CalculateModelMatrix(glm::vec3 position, glm::vec3 eulerAngles, glm::vec3 scale) {
        return
            glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(eulerAngles.x, glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::rotate(eulerAngles.y, glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::rotate(eulerAngles.z, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), scale);
    }

    glm::mat4 CalculateModelMatrix(glm::vec3 position, glm::quat orientation, glm::vec3 scale) {
        return
            glm::translate(glm::mat4(1.0f), position)
            * glm::mat4_cast(orientation)
            * glm::scale(glm::mat4(1.0f), scale);
    }

    glm::mat4 CalculateModelMatrix(const TransformComponent &component) {
        return
            glm::translate(glm::mat4(1.0f), component.Position)
            * glm::mat4_cast(component.Orientation)
            * glm::scale(glm::mat4(1.0f), component.Scale);
    }

    // Custom implementation of the LookAt function
    glm::mat4 CalculateLookAtMatrix(glm::vec3 position, glm::vec3 target, glm::vec3 worldUp)
    {
        // 1. Position = known
        // 2. Calculate cameraDirection
        glm::vec3 zaxis = glm::normalize(position - target);
        // 3. Get positive right axis vector
        glm::vec3 xaxis = glm::normalize(glm::cross(glm::normalize(worldUp), zaxis));
        // 4. Calculate camera up vector
        glm::vec3 yaxis = glm::cross(zaxis, xaxis);

        // Create translation and rotation matrix
        // In glm we access elements as mat[col][row] due to column-major layout
        glm::mat4 translation = glm::mat4(1.0f); // Identity matrix by default
        translation[3][0] = -position.x; // Fourth column, first row
        translation[3][1] = -position.y;
        translation[3][2] = -position.z;
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation[0][0] = xaxis.x; // First column, first row
        rotation[1][0] = xaxis.y;
        rotation[2][0] = xaxis.z;
        rotation[0][1] = yaxis.x; // First column, second row
        rotation[1][1] = yaxis.y;
        rotation[2][1] = yaxis.z;
        rotation[0][2] = zaxis.x; // First column, third row
        rotation[1][2] = zaxis.y;
        rotation[2][2] = zaxis.z;

        // Return lookAt matrix as combination of translation and rotation matrix
        return rotation * translation; // Remember to read from right to left (first translation then rotation)
    }


    /*
            * glm::rotate(eulerAngles.x, glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::rotate(eulerAngles.y, glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::rotate(eulerAngles.z, glm::vec3(0.0f, 0.0f, 1.0f))
    */
    glm::quat RotateByWorldAxisX(glm::quat orientation, float angle)
    {
        glm::quat modified = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(1, 0, 0));
        glm::quat result = modified * orientation;
        return result;
    }
    glm::quat RotateByWorldAxisY(glm::quat orientation, float angle)
    {
        glm::quat modified = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        glm::quat result = modified * orientation;
        return result;
    }
    glm::quat RotateByWorldAxisZ(glm::quat orientation, float angle)
    {
        glm::quat modified = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 0, 1));
        glm::quat result = modified * orientation;
        return result;
    }

    glm::quat RotateByLocalAxisX(glm::quat orientation, float angle)
    {
        glm::quat rotator = glm::angleAxis(angle, glm::vec3(1, 0, 0));
        return orientation * rotator;
    }

    glm::quat RotateByLocalAxisY(glm::quat orientation, float angle)
    {
        glm::quat rotator = glm::angleAxis(angle, glm::vec3(0, 1, 0));
        return orientation * rotator;
    }

    glm::quat RotateByLocalAxisZ(glm::quat orientation, float angle)
    {
        glm::quat rotator = glm::angleAxis(angle, glm::vec3(0, 0, 1));
        return orientation * rotator;
    }

    glm::vec3 CalculateForwardVector(const glm::quat& orientation)
    {
        glm::vec3 forward = orientation * glm::vec3(0, 0, 1);
        return forward;
    }

    glm::vec3 CalculateRightVector(const glm::quat& orientation)
    {
        glm::vec3 right = orientation * glm::vec3(1, 0, 0);
        return right;
    }

    glm::vec3 CalculateUpVector(const glm::quat& orientation)
    {
        glm::vec3 up = orientation * glm::vec3(0, 1, 0);
        return up;
    }

    glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest)
    {
        start = normalize(start);
        dest = normalize(dest);

        float cosTheta = dot(start, dest);
        glm::vec3 rotationAxis;

        if (cosTheta < -1 + 0.001f) {
            // special case when vectors in opposite directions:
            // there is no "ideal" rotation axis
            // So guess one; any will do as long as it's perpendicular to start
            rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
            if (glm::length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
                rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

            rotationAxis = normalize(rotationAxis);
            return glm::angleAxis(glm::radians(180.0f), rotationAxis);
        }

        rotationAxis = cross(start, dest);

        float s = sqrt((1 + cosTheta) * 2);
        float invs = 1 / s;

        return glm::quat(
            s * 0.5f,
            rotationAxis.x * invs,
            rotationAxis.y * invs,
            rotationAxis.z * invs
        );

    }

    glm::quat LookAtQuaternion(glm::vec3 direction, glm::vec3 desiredUp, bool forceUp = true)
    {
        // Find the rotation between the front of the object (that we assume towards +Z,
        // but this depends on your model) and the desired direction
        glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 0.0f, 1.0f), direction);


        if (forceUp)
        {
            // Recompute desiredUp so that it's perpendicular to the direction
            // You can skip that part if you really want to force desiredUp
            glm::vec3 right = glm::cross(direction, desiredUp);
            desiredUp = cross(right, direction);

            // Because of the 1rst rotation, the up is probably completely screwed up.
            // Find the rotation between the "up" of the rotated object, and the desired up
            glm::vec3 newUp = rot1 * glm::vec3(0.0f, 1.0f, 0.0f);
            glm::quat rot2 = RotationBetweenVectors(newUp, desiredUp);
            glm::quat targetOrientation = rot2 * rot1; // remember, in reverse order.

            return targetOrientation;
        }
        return rot1;
    }

    //CurrentOrientation = RotateTowards(CurrentOrientation, TargetOrientation, 3.14f * deltaTime );
    glm::quat RotateTowards(glm::quat q1, glm::quat q2, float maxAngle) {

        if (maxAngle < 0.001f) {
            // No rotation allowed. Prevent dividing by 0 later.
            return q1;
        }

        float cosTheta = dot(q1, q2);

        // q1 and q2 are already equal.
        // Force q2 just to be sure
        if (cosTheta > 0.9999f) {
            return q2;
        }

        // Avoid taking the long path around the sphere
        if (cosTheta < 0) {
            q1 = q1 * -1.0f;
            cosTheta *= -1.0f;
        }

        float angle = acos(cosTheta);

        // If there is only a 2&deg; difference, and we are allowed 5&deg;,
        // then we arrived.
        if (angle < maxAngle) {
            return q2;
        }

        float fT = maxAngle / angle;
        angle = maxAngle;

        glm::quat res = (sin((1.0f - fT) * angle) * q1 + sin(fT * angle) * q2) / sin(angle);
        res = normalize(res);
        return res;

    }
};
