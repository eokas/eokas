#include "./header.h"

namespace eokas
{
    static Quaternion hamilton(const Quaternion& a, const Quaternion& b)
    {
        return Quaternion(
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
    }

    static Vector3 rotateVec(const Quaternion& q, const Vector3& v)
    {
        Quaternion qv(v.x, v.y, v.z, 0);
        Quaternion out = hamilton(hamilton(q, qv), q.conjugate());
        return Vector3(out.x, out.y, out.z);
    }

    static Matrix4 rotationMatrix(const Quaternion& q)
    {
        float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
        return Matrix4(
            1 - 2 * (yy + zz), 2 * (xy + wz),     2 * (xz - wy),     0,
            2 * (xy - wz),     1 - 2 * (xx + zz), 2 * (yz + wx),     0,
            2 * (xz + wy),     2 * (yz - wx),     1 - 2 * (xx + yy), 0,
            0, 0, 0, 1);
    }

    Matrix4 Transform::toMatrix() const
    {
        Matrix4 s = Matrix4::scale(scale);
        Matrix4 r = rotationMatrix(rotation);
        Matrix4 t = Matrix4::translate(position);
        return Matrix4::transform(s, Matrix4::transform(r, t));
    }

    Vector3 Transform::right() const
    {
        return rotateVec(rotation, Vector3(1, 0, 0));
    }

    Vector3 Transform::up() const
    {
        return rotateVec(rotation, Vector3(0, 1, 0));
    }

    Vector3 Transform::forward() const
    {
        return rotateVec(rotation, Vector3(0, 0, 1));
    }
}
