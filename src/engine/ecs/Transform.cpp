//
// Created by redkc on 30/01/2024.
//

#include "Transform.h"

using namespace engine::ecs;

glm::mat4 Transform::getLocalModelMatrix() {
    glm::mat4 rotationMatrix = glm::toMat4(m_quaternion);
    // translation * rotation * scale (also know as TRS matrix)
    return glm::translate(glm::mat4(1.0f), m_pos) * rotationMatrix * glm::scale(glm::mat4(1.0f), m_scale);
}


glm::mat4 Transform::getLocalTranslationMatrix() {
    glm::mat4 rotationMatrix = glm::toMat4(m_quaternion);


    // translation * rotation * scale (also know as TRS matrix)
    return glm::translate(glm::mat4(1.0f), m_pos) * rotationMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(1));
}

void Transform::computeModelMatrix() {
    m_modelMatrix = getLocalModelMatrix();
    m_isDirty = false;
}

void Transform::computeModelMatrix(const glm::mat4 &parentGlobalModelMatrix) {
    m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
    m_parentMatrix = parentGlobalModelMatrix;
    m_isDirty = false;
}

void Transform::setLocalPosition(const glm::vec3 &newPosition) {
    m_pos = newPosition;
    m_isDirty = true;
}

void Transform::setLocalRotation(const glm::quat &newRotation) {
    m_quaternion = newRotation;
    m_isDirty = true;
}

void Transform::setLocalScale(const glm::vec3 &newScale) {
    m_scale = newScale;
    m_isDirty = true;
}

const glm::vec3 Transform::getGlobalPosition() const { return m_modelMatrix[3]; }

const glm::vec3 &Transform::getLocalPosition() const {
    return m_pos;
}

const glm::quat &Transform::getLocalRotation() const {
    return m_quaternion;
}

const glm::vec3 &Transform::getLocalScale() const {
    return m_scale;
}

const glm::mat4 &Transform::getModelMatrix() const {
    return m_modelMatrix;
}

glm::vec3 Transform::getRight() const {
    return m_modelMatrix[0];
}

glm::vec3 Transform::getUp() const {
    return m_modelMatrix[1];
}

glm::vec3 Transform::getBackward() const {
    return m_modelMatrix[2];
}

glm::vec3 Transform::getForward() const {
    return -m_modelMatrix[2];
}

glm::vec3 Transform::getGlobalScale() const {
    return {glm::length(getRight()), glm::length(getUp()), glm::length(getBackward())};
}

bool Transform::isDirty() const {
    return m_isDirty;
}

const glm::quat &Transform::getGlobalRotation() const {
    return glm::quat_cast(m_modelMatrix);
}

void decomposeMtx(const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale)
{
    pos = m[3];
    for(int i = 0; i < 3; i++)
        scale[i] = glm::length(glm::vec3(m[i]));
    const glm::mat3 rotMtx(
            glm::vec3(m[0]) / scale[0],
            glm::vec3(m[1]) / scale[1],
            glm::vec3(m[2]) / scale[2]);
    rot = glm::quat_cast(rotMtx);
}

void Transform::setLocalMatrix(const glm::mat4 &transformMatrix) {
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;

    decomposeMtx(transformMatrix,translation, rotation, scale);
  //  translation.z += 2;
 //   translation.y += 0.5;
    setLocalRotation(rotation);
    setLocalPosition(translation);
    setLocalScale(scale);
}



