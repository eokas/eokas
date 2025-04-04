
#include "./math.h"
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cassert>
#include <algorithm>

#include "random.h"

namespace eokas {
    /*
    ============================================================
    == Vector2
    ============================================================
    */
    const Vector2 Vector2::ZERO(0, 0);
    const Vector2 Vector2::ONE(1, 1);
    const Vector2 Vector2::UP(0, 1);
    const Vector2 Vector2::DOWN(0, -1);
    const Vector2 Vector2::RIGHT(1, 0);
    const Vector2 Vector2::LEFT(-1, 0);
    
    Vector2 Vector2::random(f32_t scale) {
        f32_t angle = Random::value() * Math::PI_MUL_2;
        Vector2 out;
        out.x = std::cos(angle) * scale;
        out.y = std::sin(angle) * scale;
        return out;
    }
    
    f32_t Vector2::dot(const Vector2& v1, const Vector2& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    
    f32_t Vector2::cosA(const Vector2& v1, const Vector2& v2) {
        f32_t dot12 = Vector2::dot(v1, v2);
        f32_t len1 = v1.magnitude();
        f32_t len2 = v2.magnitude();
        return dot12 / (len1 * len2);
    }
    
    f32_t Vector2::radians(const Vector2& v1, const Vector2& v2) {
        f32_t cosA = Vector2::cosA(v1, v2);
        return std::acos(cosA);
    }
    
    f32_t Vector2::angles(const Vector2& v1, const Vector2& v2) {
        f32_t r = Vector2::radians(v1, v2);
        return Math::radianToAngle(r);
    }
    
    Vector2::Vector2()
        : x(0), y(0) {
    }
    
    Vector2::Vector2(f32_t x, f32_t y)
        : x(x), y(y) {
    }
    
    Vector2::Vector2(const f32_t (& v)[2])
        : x(v[0]), y(v[1]) {
    }
    
    Vector2::Vector2(const Vector2& v)
        : x(v.x), y(v.y) {
    }
    
    Vector2::~Vector2() {
    }
    
    Vector2& Vector2::operator=(const Vector2& v) {
        x = v.x;
        y = v.y;
        return *this;
    }
    
    Vector2 Vector2::operator-() const {
        return Vector2(-x, -y);
    }
    
    Vector2 Vector2::operator+(const Vector2& v) const {
        return Vector2(x + v.x, y + v.y);
    }
    
    Vector2 Vector2::operator-(const Vector2& v) const {
        return Vector2(x - v.x, y - v.y);
    }
    
    Vector2 Vector2::operator*(const Vector2& v) const {
        return Vector2(x * v.x, y * v.y);
    }
    
    Vector2 Vector2::operator*(f32_t t) const {
        return Vector2(x * t, y * t);
    }
    
    Vector2& Vector2::operator+=(const Vector2& v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    
    Vector2& Vector2::operator-=(const Vector2& v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }
    
    Vector2& Vector2::operator*=(const Vector2& v) {
        x *= v.x;
        y *= v.y;
        return *this;
    }
    
    Vector2& Vector2::operator*=(f32_t t) {
        x *= t;
        y *= t;
        return *this;
    }
    
    bool Vector2::operator==(const Vector2& v) const {
        return _FloatEqual(x, v.x) && _FloatEqual(y, v.y);
    }
    
    bool Vector2::operator!=(const Vector2& v) const {
        return _FloatNotEqual(x, v.x) || _FloatNotEqual(y, v.y);
    }
    
    f32_t Vector2::sqrmagnitude() const {
        return x * x + y * y;
    }
    
    f32_t Vector2::magnitude() const {
        return sqrt(x * x + y * y);
    }
    
    Vector2 Vector2::normalized() const {
        f32_t len = sqrt(x * x + y * y);
        len = (_FloatEqual(len, 0.0f)) ? 0.0001f : len;
        return Vector2(x / len, y / len);
    }
    
    Vector2& Vector2::normalize() {
        f32_t len = sqrt(x * x + y * y);
        len = (_FloatEqual(len, 0.0f)) ? 0.0001f : len;
        x /= len;
        y /= len;
        return *this;
    }
    
    /*
    ============================================================
    == Vector3
    ============================================================
    */
    const Vector3 Vector3::ZERO(0, 0, 0);
    const Vector3 Vector3::ONE(1, 1, 1);
    const Vector3 Vector3::UP(0, 1, 0);
    const Vector3 Vector3::DOWN(0, -1, 0);
    const Vector3 Vector3::RIGHT(1, 0, 0);
    const Vector3 Vector3::LEFT(-1, 0, 0);
    const Vector3 Vector3::FORWARD(0, 0, 1);
    const Vector3 Vector3::BACK(0, 0, -1);
    
    Vector3 Vector3::random(f32_t scale) {
        f32_t angle = Random::value() * Math::PI_MUL_2;
        f32_t z = Random::range(-1.0f, 1.0f);
        f32_t xyScale = std::sqrt(1 - z * z) * scale;
        
        Vector3 out;
        out.x = std::cos(angle) * xyScale;
        out.y = std::sin(angle) * xyScale;
        out.z = z * scale;
        return out;
    }
    
    Vector3 Vector3::cross(const Vector3& v1, const Vector3& v2) {
        //(y1z2 - z1y2, z1x2 - x1z2, x1y2 - y1x2)
        return Vector3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
    }
    
    f32_t Vector3::dot(const Vector3& v1, const Vector3& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }
    
    f32_t Vector3::cosA(const Vector3& v1, const Vector3& v2) {
        f32_t dot12 = Vector3::dot(v1, v2);
        f32_t len1 = v1.magnitude();
        f32_t len2 = v2.magnitude();
        return dot12 / (len1 * len2);
    }
    
    f32_t Vector3::radians(const Vector3& v1, const Vector3& v2) {
        f32_t cosA = Vector3::cosA(v1, v2);
        return std::acos(cosA);
    }
    
    f32_t Vector3::angles(const Vector3& v1, const Vector3& v2) {
        f32_t r = Vector3::radians(v1, v2);
        return Math::radianToAngle(r);
    }
    
    Vector3::Vector3()
        : x(0), y(0), z(0) {
    }
    
    Vector3::Vector3(f32_t x, f32_t y, f32_t z)
        : x(x), y(y), z(z) {
    }
    
    Vector3::Vector3(const f32_t (& v)[3])
        : x(v[0]), y(v[1]), z(v[2]) {
    }
    
    Vector3::Vector3(const Vector2& v)
        : x(v.x), y(v.y), z(1) {
    }
    
    Vector3::Vector3(const Vector3& v)
        : x(v.x), y(v.y), z(v.z) {
    }
    
    Vector3::~Vector3() {
    }
    
    Vector3& Vector3::operator=(const Vector3& v) {
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }
    
    Vector3 Vector3::operator-() const {
        return Vector3(-x, -y, -z);
    }
    
    Vector3 Vector3::operator+(const Vector3& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }
    
    Vector3 Vector3::operator-(const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }
    
    Vector3 Vector3::operator*(const Vector3& v) const {
        return Vector3(x * v.x, y * v.y, z * v.z);
    }
    
    Vector3 Vector3::operator*(f32_t t) const {
        return Vector3(x * t, y * t, z * t);
    }
    
    Vector3& Vector3::operator+=(const Vector3& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    
    Vector3& Vector3::operator-=(const Vector3& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    
    Vector3& Vector3::operator*=(const Vector3& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }
    
    Vector3& Vector3::operator*=(f32_t t) {
        x *= t;
        y *= t;
        z *= t;
        return *this;
    }
    
    bool Vector3::operator==(const Vector3& v) const {
        return _FloatEqual(x, v.x) && _FloatEqual(y, v.y) && _FloatEqual(z, v.z);
    }
    
    bool Vector3::operator!=(const Vector3& v) const {
        return _FloatNotEqual(x, v.x) || _FloatNotEqual(y, v.y) || _FloatNotEqual(z, v.z);
    }
    
    f32_t Vector3::sqrmagnitude() const {
        return x * x + y * y + z * z;
    }
    
    f32_t Vector3::magnitude() const {
        return sqrt(x * x + y * y + z * z);
    }
    
    Vector3 Vector3::normalized() const {
        f32_t len = sqrt(x * x + y * y + z * z);
        len = (_FloatEqual(len, 0.0f)) ? 0.0001f : len;
        return Vector3(x / len, y / len, z / len);
    }
    
    Vector3& Vector3::normalize() {
        f32_t len = sqrt(x * x + y * y + z * z);
        len = (_FloatEqual(len, 0.0f)) ? 0.0001f : len;
        x /= len;
        y /= len;
        z /= len;
        return *this;
    }
    
    /*
    ============================================================
    == Vector4
    ============================================================
    */
    const Vector4 Vector4::ZERO(0, 0, 0, 0);
    const Vector4 Vector4::ONE(1, 1, 1, 1);
    
    f32_t Vector4::dot(const Vector4& v1, const Vector4& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
    }
    
    Vector4::Vector4()
        : x(0), y(0), z(0), w(0) {
    }
    
    Vector4::Vector4(f32_t x, f32_t y, f32_t z, f32_t w)
        : x(x), y(y), z(z), w(w) {
    }
    
    Vector4::Vector4(const f32_t (& v)[4])
        : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {
    }
    
    Vector4::Vector4(const Vector3& v)
        : x(v.x), y(v.y), z(v.z), w(1) {
    }
    
    Vector4::Vector4(const Vector4& v)
        : x(v.x), y(v.y), z(v.z), w(v.w) {
    }
    
    Vector4::~Vector4() {
    }
    
    Vector4& Vector4::operator=(const Vector4& v) {
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
        return *this;
    }
    
    Vector4 Vector4::operator-() const {
        return Vector4(-x, -y, -z, -w);
    }
    
    Vector4 Vector4::operator+(const Vector4& v) const {
        return Vector4(x + v.x, y + v.y, z + v.z, w + v.w);
    }
    
    Vector4 Vector4::operator-(const Vector4& v) const {
        return Vector4(x - v.x, y - v.y, z - v.z, w - v.w);
    }
    
    Vector4 Vector4::operator*(const Vector4& v) const {
        return Vector4(x * v.x, y * v.y, z * v.z, w * v.w);
    }
    
    Vector4 Vector4::operator*(f32_t t) const {
        return Vector4(x * t, y * t, z * t, w * t);
    }
    
    Vector4& Vector4::operator+=(const Vector4& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }
    
    Vector4& Vector4::operator-=(const Vector4& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }
    
    Vector4& Vector4::operator*=(const Vector4& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
    }
    
    Vector4& Vector4::operator*=(f32_t t) {
        x *= t;
        y *= t;
        z *= t;
        w *= t;
        return *this;
    }
    
    bool Vector4::operator==(const Vector4& v) const {
        return _FloatEqual(x, v.x) && _FloatEqual(y, v.y) && _FloatEqual(z, v.z) && _FloatEqual(w, v.w);
    }
    
    bool Vector4::operator!=(const Vector4& v) const {
        return _FloatNotEqual(x, v.x) || _FloatNotEqual(y, v.y) || _FloatNotEqual(z, v.z) || _FloatNotEqual(w, v.w);
    }
    
    f32_t Vector4::sqrmagnitude() const {
        return x * x + y * y + z * z + w * w;
    }
    
    f32_t Vector4::magnitude() const {
        return sqrt(x * x + y * y + z * z + w * w);
    }
    
    Vector4 Vector4::normalized() const {
        f32_t len = sqrt(x * x + y * y + z * z + w * w);
        len = (_FloatEqual(len, 0.0f)) ? 0.0001f : len;
        return Vector4(x / len, y / len, z / len, w / len);
    }
    
    Vector4& Vector4::normalize() {
        f32_t len = sqrt(x * x + y * y + z * z + w * w);
        len = (_FloatEqual(len, 0.0f)) ? 0.0001f : len;
        x /= len;
        y /= len;
        z /= len;
        w /= len;
        return *this;
    }
	
	/*
    ============================================================
    == Matrix2
    ============================================================
    */
	const Matrix2 Matrix2::ZERO(0, 0, 0, 0);
	const Matrix2 Matrix2::IDENTITY(1, 0, 0, 1);
	
	Matrix2::Matrix2() {
		memset(value, 0, sizeof(value));
	}
	
	Matrix2::Matrix2(f32_t _00, f32_t _01, f32_t _10, f32_t _11) {
		value[0][0] = _00;
		value[0][1] = _01;
		value[1][0] = _10;
		value[1][1] = _11;
	}
	
	Matrix2::Matrix2(const f32_t (& m)[2][2]) {
		memcpy(value, m, sizeof(value));
	}
	
	Matrix2::Matrix2(const Matrix2& m) {
		memcpy(value, m.value, sizeof(value));
	}
	
	Matrix2::~Matrix2() {
	}
	
	Matrix2& Matrix2::operator=(const Matrix2& m) {
		memcpy(value, m.value, sizeof(value));
		return *this;
	}
	
	Matrix2 Matrix2::operator-() const {
		return Matrix2{-value[0][0], -value[0][1], -value[1][0], -value[1][1]};
	}
	
	Matrix2 Matrix2::operator+(const Matrix2& m) const {
		return Matrix2{
			value[0][0] + m.value[0][0],
			value[0][1] + m.value[0][1],
			value[1][0] + m.value[1][0],
			value[1][1] + m.value[1][1]
		};
	}
	
	Matrix2 Matrix2::operator-(const Matrix2& m) const {
		return Matrix2{
			value[0][0] - m.value[0][0],
			value[0][1] - m.value[0][1],
			value[1][0] - m.value[1][0],
			value[1][1] - m.value[1][1]
		};
	}
	
	Matrix2 Matrix2::operator*(const Matrix2& m) const {
		return Matrix2(
			value[0][0] * m.value[0][0],
			value[0][1] * m.value[0][1],
			value[1][0] * m.value[1][0],
			value[1][1] * m.value[1][1]
		);
	}
	
	Matrix2 Matrix2::operator*(f32_t x) const {
		return Matrix2(
			value[0][0] * x,
			value[0][1] * x,
			value[1][0] * x,
			value[1][1] * x
		);
	}
	
	Matrix2& Matrix2::operator+=(const Matrix2& m) {
		value[0][0] += m.value[0][0];
		value[0][1] += m.value[0][1];
		value[1][0] += m.value[1][0];
		value[1][1] += m.value[1][1];
		return *this;
	}
	
	Matrix2& Matrix2::operator-=(const Matrix2& m) {
		value[0][0] -= m.value[0][0];
		value[0][1] -= m.value[0][1];
		value[1][0] -= m.value[1][0];
		value[1][1] -= m.value[1][1];
		return *this;
	}
	
	Matrix2& Matrix2::operator*=(const Matrix2& m) {
		value[0][0] *= m.value[0][0];
		value[0][1] *= m.value[0][1];
		value[1][0] *= m.value[1][0];
		value[1][1] *= m.value[1][1];
		return *this;
	}
	
	Matrix2& Matrix2::operator*=(f32_t x) {
		value[0][0] *= x;
		value[0][1] *= x;
		value[1][0] *= x;
		value[1][1] *= x;
		return *this;
	}
	
	bool Matrix2::operator==(const Matrix2& m) const {
		return
			_FloatEqual(value[0][0], m.value[0][0]) &&
			_FloatEqual(value[0][1], m.value[0][1]) &&
			_FloatEqual(value[1][0], m.value[1][0]) &&
			_FloatEqual(value[1][1], m.value[1][1]);
	}
	
	bool Matrix2::operator!=(const Matrix2& m) const {
		return !(*this == m);
	}
	
	f32_t Matrix2::confactor(i32_t i, i32_t j) const {
		i32_t sign = (i + j) % 2 == 0 ? 1 : -1;
		i32_t row = 1 - i;
		i32_t col = 1 - i;
		return value[row][col] * sign;
	}
	
	f32_t Matrix2::determinant() const {
		return value[0][0] * value[1][1] - value[0][1] * value[1][0];
	}
	
	Matrix2 Matrix2::transposed() const {
		return Matrix2{
			value[0][0], value[1][0],
			value[0][1], value[1][1]
		};
	}
	
	Matrix2& Matrix2::transpose() {
		f32_t temp = value[0][1];
		value[0][1] = value[1][0];
		value[1][0] = temp;
		return *this;
	}
	
	Matrix2 Matrix2::adjoint() const {
		return Matrix2{
			+value[1][1], -value[0][1],
			-value[1][0], +value[0][0]
		};
	}
	
	Matrix2 Matrix2::inverse() const {
		f32_t det = this->determinant();
		if(_FloatEqual(det, 0))
			return Matrix2::ZERO;
		f32_t inv = 1.0f / det;
		return this->adjoint() * inv;
	}
	
    /*
    ============================================================
    == Matrix3
    ============================================================
    */
	const Matrix3 Matrix3::ZERO(0, 0, 0, 0, 0, 0, 0, 0, 0);
    const Matrix3 Matrix3::IDENTITY(1, 0, 0, 0, 1, 0, 0, 0, 1);
    
    Matrix3 Matrix3::translation(const Vector2& dir) {
        Matrix3 m = Matrix3::IDENTITY;
        m.value[2][0] = dir.x;
        m.value[2][1] = dir.y;
        return m;
    }
    
    /*
    | 1,  0,  0|     | cosA,  sinA,  0|     | 1,  0,  0|
    | 0,  1,  0|  X  |-sinA,  cosA,  0|  X  | 0,  1,  0|
    |-x, -y,  1|     |    0,     0,  1|     | x,  y,  1|
    */
    Matrix3 Matrix3::rotation(const Vector2& axis, f32_t angle) {
        Matrix3 m = Matrix3::IDENTITY;
        f32_t x = axis.x;
        f32_t y = axis.y;
        f32_t sinAngle = sin(angle);
        f32_t cosAngle = cos(angle);
        m.value[0][0] = cosAngle;
        m.value[0][1] = sinAngle;
        m.value[0][2] = 0.0f;
        m.value[1][0] = -sinAngle;
        m.value[1][1] = cosAngle;
        m.value[1][2] = 0.0f;
        m.value[2][0] = x * (1.0f - cosAngle) + y * sinAngle;
        m.value[2][1] = y * (1.0f - cosAngle) - x * cosAngle;
        m.value[2][2] = 1.0f;
        return m;
    }
    
    Matrix3 Matrix3::scaling(const Vector2& scale) {
        Matrix3 m = Matrix3::IDENTITY;
        m.value[0][0] = scale.x;
        m.value[1][1] = scale.y;
        return m;
    }
    
    Vector3 Matrix3::transform(const Vector3& v, const Matrix3& t) {
        Vector3 result;
        result.x = v.x * t.value[0][0] + v.y * t.value[1][0] + v.z * t.value[2][0];
        result.y = v.x * t.value[0][1] + v.y * t.value[1][1] + v.z * t.value[2][1];
        result.z = v.x * t.value[0][2] + v.y * t.value[1][2] + v.z * t.value[2][2];
        return result;
    }
    
    Matrix3 Matrix3::transform(const Matrix3& m, const Matrix3& t) {
        Matrix3 result;
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                for (i32_t k = 0; k < 3; k++) {
                    result.value[i][j] += m.value[i][k] * t.value[k][j];
                }
            }
        }
        return result;
    }
    
    Matrix3::Matrix3() {
        memset(value, 0, sizeof(value));
    }
    
    Matrix3::Matrix3(f32_t _00, f32_t _01, f32_t _02, f32_t _10, f32_t _11, f32_t _12, f32_t _20, f32_t _21, f32_t _22) {
        value[0][0] = _00;
        value[0][1] = _01;
        value[0][2] = _02;
        value[1][0] = _10;
        value[1][1] = _11;
        value[1][2] = _12;
        value[2][0] = _20;
        value[2][1] = _21;
        value[2][2] = _22;
    }
    
    Matrix3::Matrix3(const f32_t (& m)[3][3]) {
        memcpy(value, m, sizeof(value));
    }
    
    Matrix3::Matrix3(const Matrix3& m) {
        memcpy(value, m.value, sizeof(value));
    }
    
    Matrix3::~Matrix3() {
    }
    
    Matrix3& Matrix3::operator=(const Matrix3& m) {
        memcpy(value, m.value, sizeof(value));
        return *this;
    }
    
    Matrix3 Matrix3::operator-() const {
        f32_t result[3][3];
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                result[i][j] = -value[i][j];
            }
        }
        return Matrix3(result);
    }
    
    Matrix3 Matrix3::operator+(const Matrix3& m) const {
        f32_t result[3][3];
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                result[i][j] = value[i][j] + m.value[i][j];
            }
        }
        return Matrix3(result);;
    }
    
    Matrix3 Matrix3::operator-(const Matrix3& m) const {
        f32_t result[3][3];
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                result[i][j] = value[i][j] - m.value[i][j];
            }
        }
        return Matrix3(result);
    }
    
    Matrix3 Matrix3::operator*(const Matrix3& m) const {
        f32_t result[3][3];
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                result[i][j] = value[i][j] * m.value[i][j];
            }
        }
        return Matrix3(result);
    }
	
	Matrix3 Matrix3::operator*(f32_t x) const {
		f32_t result[3][3];
		for (i32_t i = 0; i < 3; i++) {
			for (i32_t j = 0; j < 3; j++) {
				result[i][j] = value[i][j] * x;
			}
		}
		return Matrix3(result);
	}
    
    Matrix3& Matrix3::operator+=(const Matrix3& m) {
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                value[i][j] += m.value[i][j];
            }
        }
        return *this;
    }
    
    Matrix3& Matrix3::operator-=(const Matrix3& m) {
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                value[i][j] -= m.value[i][j];
            }
        }
        return *this;
    }
    
    Matrix3& Matrix3::operator*=(const Matrix3& m) {
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                value[i][j] *= m.value[i][j];
            }
        }
        return *this;
    }
	
	Matrix3& Matrix3::operator*=(f32_t x) {
		for (i32_t i = 0; i < 3; i++) {
			for (i32_t j = 0; j < 3; j++) {
				value[i][j] *= x;
			}
		}
		return *this;
	}
    
    bool Matrix3::operator==(const Matrix3& m) const {
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                if (!_FloatEqual(value[i][j], m.value[i][j])) {
                    return false;
                }
            }
        }
        return true;
    }
    
    bool Matrix3::operator!=(const Matrix3& m) const {
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                if (!_FloatEqual(value[i][j], m.value[i][j])) {
                    return true;
                }
            }
        }
        return false;
    }
	
	f32_t Matrix3::confactor(i32_t i, i32_t j) const {
		Matrix2 cm;
		for(i32_t row = 0; row < 3; row++) {
			for(i32_t col = 0; col < 3; col++) {
				if(row == i || col == j)
					continue;
				i32_t r = row < i ? row : row - 1;
				i32_t c = col < j ? col : col - 1;
				cm.value[r][c] = value[row][col];
			}
		}
		
		f32_t sign = (i + j) % 2 == 0 ? 1.0f : -1.0f;
		return sign * cm.determinant();
	}
    
    /*
        a00 a01 a02 a00 a01
        a10 a11 a12 a10 a11
        a20 a21 a22 a20 a21
    
        D(A) =
            a00a11a22 + a01a12a20 + a02a10a21 -
            a02a11a20 - a00a12a21 - a01a10a22
    */
    f32_t Matrix3::determinant() const {
        return
			value[0][0] * value[1][1] * value[2][2] +
			value[0][1] * value[1][2] * value[2][0] +
			value[0][2] * value[1][0] * value[2][1] -
			value[0][2] * value[1][1] * value[2][0] -
			value[0][0] * value[1][2] * value[2][1] -
			value[0][1] * value[1][0] * value[2][2];
    }

    Matrix3 Matrix3::transposed() const {
        Matrix3 t(value);
        for (i32_t i = 1; i < 3; i++) {
            for (i32_t j = 0; j < i; j++) {
                t.value[i][j] = value[j][i];
                t.value[j][i] = value[i][j];
            }
        }
        return t;
    }
    
    Matrix3& Matrix3::transpose() {
        Matrix3 t(value);
        for (i32_t i = 1; i < 3; i++) {
            for (i32_t j = 0; j < i; j++) {
                value[i][j] = t.value[j][i];
                value[j][i] = t.value[i][j];
            }
        }
        return *this;
    }
	
	Matrix3 Matrix3::adjoint() const {
		Matrix3 adj;
		for(i32_t i = 0; i < 3; i++) {
			for(i32_t j = 0; j < 3; j++) {
				adj.value[i][j] = this->confactor(i, j);
			}
		}
		adj.transpose();
		return adj;
	}
	
	Matrix3 Matrix3::inverse() const {
		f32_t det = this->determinant();
		if(_FloatEqual(det, 0))
			return Matrix3::ZERO;
		float inv = 1.0f / det;
		return this->adjoint() * inv;
	}
    
    /*
    ============================================================
    == Matrix4
    ============================================================
    */
	const Matrix4 Matrix4::ZERO(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    const Matrix4 Matrix4::IDENTITY(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    
    Matrix4 Matrix4::translate(const Vector3& dir) {
        Matrix4 m = Matrix4::IDENTITY;
        m.value[3][0] = dir.x;
        m.value[3][1] = dir.y;
        m.value[3][2] = dir.z;
        return m;
    }
    
    Matrix4 Matrix4::rotate(const Vector3& axis, f32_t angle) {
        Matrix4 m = Matrix4::IDENTITY;
        f32_t x = axis.x;
        f32_t y = axis.y;
        f32_t z = axis.z;
        f32_t sinAngle = sin(angle);
        f32_t cosAngle = cos(angle);
        m.value[0][0] = (x * x) * (1.0f - cosAngle) + cosAngle;
        m.value[1][0] = (x * y) * (1.0f - cosAngle) - (z * sinAngle);
        m.value[2][0] = (x * z) * (1.0f - cosAngle) + (y * sinAngle);
        m.value[0][1] = (y * x) * (1.0f - cosAngle) + (z * sinAngle);
        m.value[1][1] = (y * y) * (1.0f - cosAngle) + cosAngle;
        m.value[2][1] = (y * z) * (1.0f - cosAngle) - (x * sinAngle);
        m.value[0][2] = (z * x) * (1.0f - cosAngle) - (y * sinAngle);
        m.value[1][2] = (z * y) * (1.0f - cosAngle) + (x * sinAngle);
        m.value[2][2] = (z * z) * (1.0f - cosAngle) + cosAngle;
        return m;
    }
    
    Matrix4 Matrix4::scale(const Vector3& scale) {
        Matrix4 m = Matrix4::IDENTITY;
        m.value[0][0] = scale.x;
        m.value[1][1] = scale.y;
        m.value[2][2] = scale.z;
        return m;
    }
    
    Matrix4 Matrix4::lookToLH(const Vector3& pos, const Vector3& forward, const Vector3& up) {
        assert(forward != Vector3(0, 0, 0));
        assert(up != Vector3(0, 0, 0));
        
        Vector3 r2 = forward.normalized();
        Vector3 r0 = Vector3::cross(up, r2);
        r0.normalize();
        Vector3 r1 = Vector3::cross(r2, r0);
        
        Vector3 negpos = -pos;
        f32_t d0 = Vector3::dot(r0, negpos);
        f32_t d1 = Vector3::dot(r1, negpos);
        f32_t d2 = Vector3::dot(r2, negpos);
        
        Matrix4 m;
        Math::fill(m.value[0], r0.x, r0.y, r0.z, d0);
        Math::fill(m.value[1], r1.x, r1.y, r1.z, d1);
        Math::fill(m.value[2], r2.x, r2.y, r2.z, d2);
        Math::fill(m.value[3], 0.0f, 0.0f, 0.0f, 1.0f);
        m.transpose();
        return m;
    }
    
    Matrix4 Matrix4::lookAtLH(const Vector3& pos, const Vector3& focus, const Vector3& up) {
        Vector3 forward = focus - pos;
        return Matrix4::lookToLH(pos, forward, up);
    }
    
    Matrix4 Matrix4::lookToRH(const Vector3& pos, const Vector3& forward, const Vector3& up) {
        Vector3 back = -forward;
        return Matrix4::lookToLH(pos, back, up);
    }
    
    Matrix4 Matrix4::lookAtRH(const Vector3& pos, const Vector3& focus, const Vector3& up) {
        Vector3 back = pos - focus;
        return Matrix4::lookToLH(pos, back, up);
    }
    
    Matrix4 Matrix4::orthographicLH(f32_t width, f32_t height, f32_t near, f32_t far) {
        assert(_FloatNotEqual(width, 0));
        assert(_FloatNotEqual(height, 0));
        assert(_FloatNotEqual(near, far));
        
        f32_t range = 1.0f / (far - near);
        
        Matrix4 m;
        Math::fill(m.value[0], 2.0f / width, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, 2.0f / height, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 0.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, -range * near, 1.0f);
        return m;
    }
    
    Matrix4 Matrix4::orthographicRH(f32_t width, f32_t height, f32_t near, f32_t far) {
        assert(_FloatNotEqual(width, 0));
        assert(_FloatNotEqual(height, 0));
        assert(_FloatNotEqual(near, far));
        
        f32_t range = 1.0f / (near - far);
        
        Matrix4 m;
        Math::fill(m.value[0], 2.0f / width, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, 2.0f / height, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 0.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, range * near, 1.0f);
        return m;
    }
    
    Matrix4 Matrix4::orthographicOffCenterLH(f32_t left, f32_t right, f32_t top, f32_t bottom, f32_t near, f32_t far) {
        assert(_FloatNotEqual(left, right));
        assert(_FloatNotEqual(top, bottom));
        assert(_FloatNotEqual(near, far));
        
        f32_t reciprocalWidth = 1.0f / (right - left);
        f32_t reciprocalHeight = 1.0f / (top - bottom);
        f32_t range = 1.0f / (far - near);
        
        Matrix4 m;
        Math::fill(m.value[0], reciprocalHeight + reciprocalWidth, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, reciprocalHeight + reciprocalHeight, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 0.0f);
        Math::fill(m.value[3], -(left + right) * reciprocalWidth, -(top + bottom) * reciprocalHeight, -range * near, 1.0f);
        return m;
    }
    
    Matrix4 Matrix4::orthographicOffCenterRH(f32_t left, f32_t right, f32_t top, f32_t bottom, f32_t near, f32_t far) {
        assert(_FloatNotEqual(left, right));
        assert(_FloatNotEqual(top, bottom));
        assert(_FloatNotEqual(near, far));
        
        f32_t reciprocalWidth = 1.0f / (right - left);
        f32_t reciprocalHeight = 1.0f / (top - bottom);
        f32_t range = 1.0f / (near - far);
        
        Matrix4 m;
        Math::fill(m.value[0], reciprocalHeight + reciprocalWidth, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, reciprocalHeight + reciprocalHeight, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 0.0f);
        Math::fill(m.value[3], -(left + right) * reciprocalWidth, -(top + bottom) * reciprocalHeight, range * near, 1.0f);
        return m;
    }
    
    Matrix4 Matrix4::perspectiveLH(f32_t width, f32_t height, f32_t near, f32_t far) {
        assert(_FloatNotEqual(width, 0.0f));
        assert(_FloatNotEqual(height, 0.0f));
        assert(_FloatNotEqual(near, far));
        
        f32_t twoNear = near + near;
        f32_t range = far / (far - near);
        
        Matrix4 m;
        Math::fill(m.value[0], twoNear / width, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, twoNear / height, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 1.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, -range * near, 0.0f);
        return m;
    }
    
    Matrix4 Matrix4::perspectiveRH(f32_t width, f32_t height, f32_t near, f32_t far) {
        assert(_FloatNotEqual(width, 0.0f));
        assert(_FloatNotEqual(height, 0.0f));
        assert(_FloatNotEqual(near, far));
        
        f32_t twoNear = near + near;
        f32_t range = far / (near - far);
        
        Matrix4 m;
        Math::fill(m.value[0], twoNear / width, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, twoNear / height, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, -1.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, range * near, 0.0f);
        return m;
    }
    
    Matrix4 Matrix4::perspectiveFovLH(f32_t fov, f32_t aspect, f32_t near, f32_t far) {
        assert(_FloatNotEqual(fov, 0.0f));
        assert(_FloatNotEqual(aspect, 0.0f));
        assert(_FloatNotEqual(near, far));
        
        f32_t sinFov = std::sin(fov * 0.5f);
        f32_t cosFov = std::cos(fov * 0.5f);
        f32_t height = cosFov / sinFov;
        f32_t width = height / aspect;
        f32_t range = far / (far - near);
        
        Matrix4 m;
        Math::fill(m.value[0], width, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, height, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 1.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, -range * near, 0.0f);
        return m;
    }
    
    Matrix4 Matrix4::perspectiveFovRH(f32_t fov, f32_t aspect, f32_t near, f32_t far) {
        assert(_FloatNotEqual(fov, 0.0f));
        assert(_FloatNotEqual(aspect, 0.0f));
        assert(_FloatNotEqual(near, far));
        
        f32_t sinFov = std::sin(fov * 0.5f);
        f32_t cosFov = std::cos(fov * 0.5f);
        f32_t height = cosFov / sinFov;
        f32_t width = height / aspect;
        f32_t range = far / (near - far);
        
        Matrix4 m;
        Math::fill(m.value[0], width, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, height, 0.0f, 0.0f);
        Math::fill(m.value[2], 0.0f, 0.0f, range, 1.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, range * near, 0.0f);
        return m;
    }
    
    Matrix4 Matrix4::perspectiveOffCenterLH(f32_t left, f32_t right, f32_t top, f32_t bottom, f32_t near, f32_t far) {
        assert(_FloatNotEqual(left, right));
        assert(_FloatNotEqual(top, bottom));
        assert(_FloatNotEqual(near, far));
        
        f32_t twoNear = near + near;
        f32_t reciprocalWidth = 1.0f / (right - left);
        f32_t reciprocalHeight = 1.0f / (top - bottom);
        f32_t range = far / (far - near);
        
        Matrix4 m;
        Math::fill(m.value[0], twoNear * reciprocalWidth, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, twoNear * reciprocalHeight, 0.0f, 0.0f);
        Math::fill(m.value[2], -(left + right) * reciprocalWidth, -(top + bottom) * reciprocalHeight, range, 1.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, -range * near, 0.0f);
        return m;
    }
    
    Matrix4 Matrix4::perspectiveOffCenterRH(f32_t left, f32_t right, f32_t top, f32_t bottom, f32_t near, f32_t far) {
        assert(_FloatNotEqual(left, right));
        assert(_FloatNotEqual(top, bottom));
        assert(_FloatNotEqual(near, far));
        
        f32_t twoNear = near + near;
        f32_t reciprocalWidth = 1.0f / (right - left);
        f32_t reciprocalHeight = 1.0f / (top - bottom);
        f32_t range = far / (near - far);
        
        Matrix4 m;
        Math::fill(m.value[0], twoNear * reciprocalWidth, 0.0f, 0.0f, 0.0f);
        Math::fill(m.value[1], 0.0f, twoNear * reciprocalHeight, 0.0f, 0.0f);
        Math::fill(m.value[2], (left + right) * reciprocalWidth, (top + bottom) * reciprocalHeight, range, -1.0f);
        Math::fill(m.value[3], 0.0f, 0.0f, range * near, 0.0f);
        return m;
    }
    
    Vector4 Matrix4::transform(const Vector4& v, const Matrix4& t) {
        Vector4 result;
        result.x = v.x * t.value[0][0] + v.y * t.value[1][0] + v.z * t.value[2][0] + v.w * t.value[3][0];
        result.y = v.x * t.value[0][1] + v.y * t.value[1][1] + v.z * t.value[2][1] + v.w * t.value[3][1];
        result.z = v.x * t.value[0][2] + v.y * t.value[1][2] + v.z * t.value[2][2] + v.w * t.value[3][2];
        result.w = v.x * t.value[0][3] + v.y * t.value[1][3] + v.z * t.value[2][3] + v.w * t.value[3][3];
        return result;
    }
    
    Matrix4 Matrix4::transform(const Matrix4& m, const Matrix4& t) {
        Matrix4 result;
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                for (i32_t k = 0; k < 4; k++) {
                    result.value[i][j] += m.value[i][k] * t.value[k][j];
                }
            }
        }
        return result;
    }
    
    Matrix4::Matrix4() {
        memset(value, 0, sizeof(value));
    }
    
    Matrix4::Matrix4(f32_t _00, f32_t _01, f32_t _02, f32_t _03, f32_t _10, f32_t _11, f32_t _12, f32_t _13, f32_t _20, f32_t _21, f32_t _22, f32_t _23, f32_t _30, f32_t _31, f32_t _32, f32_t _33) {
        value[0][0] = _00;
        value[0][1] = _01;
        value[0][2] = _02;
        value[0][3] = _03;
        value[1][0] = _10;
        value[1][1] = _11;
        value[1][2] = _12;
        value[1][3] = _13;
        value[2][0] = _20;
        value[2][1] = _21;
        value[2][2] = _22;
        value[2][3] = _23;
        value[3][0] = _30;
        value[3][1] = _31;
        value[3][2] = _32;
        value[3][3] = _33;
    }
    
    Matrix4::Matrix4(const f32_t (& m)[4][4]) {
        memcpy(value, m, sizeof(value));
    }
    
    Matrix4::Matrix4(const Matrix3& m) {
        memset(value, 0, sizeof(value));
        for (i32_t i = 0; i < 3; i++) {
            for (i32_t j = 0; j < 3; j++) {
                value[i][j] = m.value[i][j];
            }
        }
        value[3][3] = 1;
    }
    
    Matrix4::Matrix4(const Matrix4& m) {
        memcpy(value, m.value, sizeof(value));
    }
    
    Matrix4::~Matrix4() {
    }
    
    Matrix4& Matrix4::operator=(const Matrix4& m) {
        memcpy(value, m.value, sizeof(value));
        return *this;
    }
    
    Matrix4 Matrix4::operator-() const {
        f32_t result[4][4];
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                result[i][j] = -value[i][j];
            }
        }
        return Matrix4(result);
    }
    
    Matrix4 Matrix4::operator+(const Matrix4& m) const {
        f32_t result[4][4];
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                result[i][j] = value[i][j] + m.value[i][j];
            }
        }
        return Matrix4(result);;
    }
    
    Matrix4 Matrix4::operator-(const Matrix4& m) const {
        f32_t result[4][4];
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                result[i][j] = value[i][j] - m.value[i][j];
            }
        }
        return Matrix4(result);
    }
    
    Matrix4 Matrix4::operator*(const Matrix4& m) const {
        f32_t result[4][4];
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                result[i][j] = value[i][j] * m.value[i][j];
            }
        }
        return Matrix4(result);
    }
	
	Matrix4 Matrix4::operator*(f32_t x) const {
		f32_t result[4][4];
		for (i32_t i = 0; i < 4; i++) {
			for (i32_t j = 0; j < 4; j++) {
				result[i][j] = value[i][j] * x;
			}
		}
		return Matrix4(result);
	}
    
    Matrix4& Matrix4::operator+=(const Matrix4& m) {
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                value[i][j] += m.value[i][j];
            }
        }
        return *this;
    }
    
    Matrix4& Matrix4::operator-=(const Matrix4& m) {
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                value[i][j] -= m.value[i][j];
            }
        }
        return *this;
    }
    
    Matrix4& Matrix4::operator*=(const Matrix4& m) {
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                value[i][j] *= m.value[i][j];
            }
        }
        return *this;
    }
	
	Matrix4& Matrix4::operator*=(f32_t x) {
		for (i32_t i = 0; i < 4; i++) {
			for (i32_t j = 0; j < 4; j++) {
				value[i][j] *= x;
			}
		}
		return *this;
	}
    
    bool Matrix4::operator==(const Matrix4& m) const {
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                if (_FloatNotEqual(value[i][j], m.value[i][j])) {
                    return false;
                }
            }
        }
        return true;
    }
    
    bool Matrix4::operator!=(const Matrix4& m) const {
        for (i32_t i = 0; i < 4; i++) {
            for (i32_t j = 0; j < 4; j++) {
                if (_FloatNotEqual(value[i][j], m.value[i][j])) {
                    return true;
                }
            }
        }
        return false;
    }
	
	f32_t Matrix4::confactor(i32_t i, i32_t j) const {
		Matrix3 cm;
		for(i32_t row = 0; row < 4; row++) {
			for(i32_t col = 0; col < 4; col++) {
				if(row == i || col == j)
					continue;
				i32_t r = row < i ? row : row - 1;
				i32_t c = col < j ? col : col - 1;
				cm.value[r][c] = value[row][col];
			}
		}
		
		f32_t sign = (i + j) % 2 == 0 ? 1.0f : -1.0f;
		return sign * cm.determinant();
	}
    
    /*
        a00 a01 a02 a03 a00 a01 a02
        a10 a11 a12 a13 a10 a11 a12
        a20 a21 a22 a23 a20 a21 a22
        a30 a31 a32 a33 a30 a31 a32
    
        D(A) =
            a00a11a22a33 + a01a12a23a30 + a02a13a20a31 + a03a10a21a32 -
            a03a12a21a30 - a00a13a22a31 - a01a10a23a32 - a02a11a20a33
    */
    f32_t Matrix4::determinant() const {
        return
			value[0][0] * value[1][1] * value[2][2] * value[3][3] +
			value[0][1] * value[1][2] * value[2][3] * value[3][0] +
			value[0][2] * value[1][3] * value[2][0] * value[3][1] +
            value[0][3] * value[1][0] * value[2][1] * value[3][2] -
            value[0][3] * value[1][2] * value[2][1] * value[3][0] -
			value[0][0] * value[1][3] * value[2][2] * value[3][1] -
			value[0][1] * value[1][0] * value[2][3] * value[3][2] -
            value[0][2] * value[1][1] * value[2][0] * value[3][3];
    }
    
    Matrix4 Matrix4::transposed() const {
        Matrix4 t(value);
        for (i32_t i = 1; i < 4; i++) {
            for (i32_t j = 0; j < i; j++) {
                t.value[i][j] = value[j][i];
                t.value[j][i] = value[i][j];
            }
        }
        return t;
    }
    
    Matrix4& Matrix4::transpose() {
        Matrix4 t(value);
        for (i32_t i = 1; i < 4; i++) {
            for (i32_t j = 0; j < i; j++) {
                value[i][j] = t.value[j][i];
                value[j][i] = t.value[i][j];
            }
        }
        return *this;
    }
	
	Matrix4 Matrix4::adjoint() const {
		Matrix4 adj;
		for(i32_t i = 0; i < 4; i++) {
			for(i32_t j = 0; j < 4; j++) {
				adj.value[i][j] = this->confactor(i, j);
			}
		}
		adj.transpose();
		return adj;
	}
	
	Matrix4 Matrix4::inverse() const {
		f32_t det = this->determinant();
		if(_FloatEqual(det, 0))
			return Matrix4::ZERO;
		f32_t inv = 1.0f / det;
		return this->adjoint() * inv;
	}
    
    /*
    ============================================================
    == Quaternion
    ============================================================
    */
    const Quaternion Quaternion::IDENTITY(0, 0, 0, 1);
    
    Quaternion Quaternion::rotateEulerAngles(f32_t x, f32_t y, f32_t z) {
        return Quaternion();
    }
    
    Quaternion Quaternion::rotateAxisAngle(const Vector3& axis, f32_t angle) {
        Vector3 n = axis.normalized();
        f32_t a = Math::angleToRadian(angle);
        f32_t cosAv2 = cos(a * 0.5f);
        f32_t sinAv2 = sin(a * 0.5f);
        return Quaternion(n.x * sinAv2, n.y * sinAv2, n.z * sinAv2, cosAv2);
    }
    
    Quaternion Quaternion::rotateBetween(const Vector3& a, const Vector3& b) {
        return Quaternion();
    }
    
    // d = inverse(a) X b
    Quaternion Quaternion::rotateBetween(const Quaternion& a, const Quaternion& b) {
        return Quaternion();
    }
    
    Quaternion::Quaternion()
        : x(0), y(0), z(0), w(0) {
    }
    
    Quaternion::Quaternion(f32_t x, f32_t y, f32_t z, f32_t w)
        : x(x), y(y), z(z), w(w) {
    }
    
    Quaternion::Quaternion(const f32_t(& q)[4])
        : x(q[0]), y(q[1]), z(q[2]), w(q[3]) {
    }
    
    Quaternion::Quaternion(const Quaternion& q)
        : x(q.x), y(q.y), z(q.z), w(q.w) {
    }
    
    Quaternion::~Quaternion() {
    }
    
    Quaternion& Quaternion::operator=(const Quaternion& q) {
        if (*this == q)
            return *this;
        x = q.x;
        y = q.y;
        z = q.z;
        w = q.w;
        return *this;
    }
    
    Quaternion Quaternion::operator-() const {
        return Quaternion(-x, -y, -z, -w);
    }
    
    Quaternion Quaternion::operator+(const Quaternion& q) const {
        return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
    }
    
    Quaternion Quaternion::operator-(const Quaternion& q) const {
        return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
    }
    
    Quaternion Quaternion::operator*(const Quaternion& q) const {
        return Quaternion(x * q.x, y * q.y, z * q.z, w * q.w);
    }
    
    Quaternion& Quaternion::operator+=(const Quaternion& q) {
        x += q.x;
        y += q.y;
        z += q.z;
        w += q.w;
        return *this;
    }
    
    Quaternion& Quaternion::operator-=(const Quaternion& q) {
        x -= q.x;
        y -= q.y;
        z -= q.z;
        w -= q.w;
        return *this;
    }
    
    Quaternion& Quaternion::operator*=(const Quaternion& q) {
        x *= q.x;
        y *= q.y;
        z *= q.z;
        w *= q.w;
        return *this;
    }
    
    bool Quaternion::operator==(const Quaternion& q) const {
        return _FloatEqual(x, q.x) && _FloatEqual(y, q.y) && _FloatEqual(z, q.z) && _FloatEqual(w, q.w);
    }
    
    bool Quaternion::operator!=(const Quaternion& q) const {
        return _FloatNotEqual(x, q.x) || _FloatNotEqual(y, q.y) || _FloatNotEqual(z, q.z) || _FloatNotEqual(w, q.w);
    }
    
    f32_t Quaternion::sqrmagnitude() const {
        return x * x + y * y + z * z + w * w;
    }
    
    f32_t Quaternion::magnitude() const {
        return sqrt(x * x + y * y + z * z + w * w);
    }
    
    Quaternion Quaternion::conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }
    
    /*
        inverse(q) = conjugate(q) / magnitude(q)
    */
    Quaternion Quaternion::inverse() const {
        f32_t mag = sqrt(x * x + y * y + z * z + w * w);
        mag = _FloatEqual(mag, 0) ? 0.0001f : mag;
        return Quaternion(-x / mag, -y / mag, -z / mag, w / mag);
    }
    
    /*
    ============================================================
    == Spherical
    ============================================================
    */
    Spherical::Spherical()
        : radius(0), phi(0), theta(0) {
    }
    
    Spherical::Spherical(f32_t radius, f32_t phi, f32_t theta)
        : radius(radius), phi(phi), theta(theta) {
    }
    
    Spherical::Spherical(const Spherical& other)
        : radius(other.radius), phi(other.phi), theta(other.theta) {
    }
    
    Spherical::Spherical(const Vector3& point) {
        f32_t radius = point.magnitude();
        this->radius = radius;
        
        if (radius > 0) {
            this->theta = std::atan2(point.x, point.z);
            this->phi = std::acos(Math::clamp(point.y / radius, -1, 1));
        } else {
            this->theta = 0;
            this->phi = 0;
        }
    }
    
    Spherical::~Spherical() {
    }
    
    Spherical::operator Vector3() {
        f32_t radius = this->radius;
        f32_t phi = this->phi;
        f32_t theta = this->theta;
        return Vector3(radius * std::sin(phi) * std::sin(theta), radius * std::cos(phi), radius * std::sin(phi) * std::cos(theta));
    }
    
    
    /*
    ============================================================
    == Ray
    ============================================================
    */
    Ray::Ray()
        : origin(0, 0, 0), diBounds2ion(0, 1, 0) {
    }
    
    Ray::Ray(const Vector3& origin, const Vector3& diBounds2ion)
        : origin(origin), diBounds2ion(diBounds2ion) {
    }
    
    Ray::Ray(const Ray& other)
        : origin(other.origin), diBounds2ion(other.diBounds2ion) {
    }
    
    Ray::~Ray() {
    }
    
    Vector3 Ray::point(f32_t t) const {
        return Vector3(origin + diBounds2ion.normalized() * t);
    }
    
    /*
    ============================================================
    == Plane
    ============================================================
    */
    Plane::Plane()
        : normal(0, 1, 0), distance(0) {
    }
    
    Plane::Plane(const Vector3& normal, f32_t distance)
        : normal(normal), distance(distance) {
    }
    
    Plane::Plane(const Vector3& normal, const Vector3& position)
        : normal(normal), distance(0) {
        const Vector3 n = normal.normalized();
        const Vector3& p = position;
        distance = Vector3::dot(n, p);
    }
    
    Plane::Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3)
        : normal(0, 1, 0), distance(0) {
        Vector3 a = p2 - p1;
        Vector3 b = p3 - p1;
        normal = Vector3::cross(a, b);
        normal.normalize();
        distance = Vector3::dot(normal, p1);
    }
    
    Plane::Plane(const Plane& other)
        : normal(other.normal), distance(other.distance) {
    }
    
    Plane::~Plane() {
    }
    
    f32_t Plane::side(const Vector3& p) const {
        Vector3 n = normal.normalized();
        Vector3 o = n * distance;
        Vector3 m = p - o;
        return Vector3::cosA(n, m);
    }
    
    /*
    ============================================================
    == Sphere
    ============================================================
    */
    Sphere::Sphere()
        : origin(), radius(1) {
    }
    
    Sphere::Sphere(f32_t x, f32_t y, f32_t z, f32_t r)
        : origin(x, y, z), radius(r) {
    }
    
    Sphere::Sphere(const Vector3& o, f32_t r)
        : origin(o), radius(r) {
    }
    
    Sphere::Sphere(const Sphere& other)
        : origin(other.origin), radius(other.radius) {
    }
    
    Sphere::~Sphere() {
    }
    
    f32_t Sphere::side(const Vector3& p) const {
        Vector3 d = p - this->origin;
        return d.magnitude() - this->radius;
    }
    
    bool Sphere::include(const Vector3& p) const {
        return this->side(p) <= 0;
    }
    
    Sphere& Sphere::expand(const Vector3& p) {
        Vector3 dir = this->origin - p;
        f32_t dis = dir.magnitude();
        if (dis <= this->radius)
            return *this;
        
        f32_t radius = (dis + this->radius) * 0.5f;
        this->origin = p + dir.normalize() * radius;
        this->radius = radius;
        return *this;
    }
    
    /*
    ============================================================
    == Rect
    ============================================================
    */
    Rect::Rect()
        : x(0), y(0), width(0), height(0) {
    }
    
    Rect::Rect(f32_t x, f32_t y, f32_t w, f32_t h)
        : x(x), y(y), width(w), height(h) {
    }
    
    Rect::Rect(const Vector2& pos, const Vector2& size)
        : x(pos.x), y(pos.y), width(size.x), height(size.y) {
    }
    
    bool Rect::contains(const Vector2& p) {
        return p.x >= this->x && p.x <= this->x + this->width && p.y >= this->x && p.y <= this->y + this->height;
    }
    
    /*
    ============================================================
    == Bounds2
    ============================================================
    */
    Bounds2::Bounds2()
        : min(), max() {
    }
    
    Bounds2::Bounds2(const Bounds2& other)
        : min(other.min), max(other.max) {
    }
    
    Bounds2::~Bounds2() {
    }
    
    Bounds2& Bounds2::operator=(const Bounds2& other) {
        this->min = other.min;
        this->max = other.max;
        return *this;
    }
    
    f32_t Bounds2::width() const {
        return max.x - min.x;
    }
    
    void Bounds2::width(f32_t val) {
        f32_t c = (min.x + max.x) * 0.5f;
        min.x = c - val * 0.5f;
        max.x = c + val * 0.5f;
    }
    
    f32_t Bounds2::height() const {
        return max.y - min.y;
    }
    
    void Bounds2::height(f32_t val) {
        f32_t c = (min.y + max.y) * 0.5f;
        min.y = c - val * 0.5f;
        max.y = c + val * 0.5f;
    }
    
    Vector2 Bounds2::size() const {
        return max - min;
    }
    
    void Bounds2::size(const Vector2& val) {
        Vector2 c = this->center();
        min = c - val * 0.5f;
        max = c + val * 0.5f;
    }
    
    Vector2 Bounds2::center() const {
        return (min + max) * 0.5f;
    }
    
    void Bounds2::center(const Vector2& val) {
        Vector2 d = val - this->center();
        min += d;
        max += d;
    }
    
    Vector2 Bounds2::extent() const {
        return (max - min) * 0.5f;
    }
    
    void Bounds2::extent(const Vector2& val) {
        Vector2 c = this->center();
        min = c - val;
        max = c + val;
    }
    
    bool Bounds2::contains(const Vector2& p) const {
        return Math::between(p, this->min, this->max);
    }
    
    Bounds2& Bounds2::expand(const Vector2& p) {
        if (this->min.x > p.x) this->min.x = p.x;
        if (this->min.y > p.y) this->min.y = p.y;
        if (this->max.x < p.x) this->max.x = p.x;
        if (this->max.y < p.y) this->max.y = p.y;
        return *this;
    }
    
    Bounds2& Bounds2::expand(const Bounds2& b) {
        this->expand(b.min);
        this->expand(b.max);
        return *this;
    }
    
    /*
    ============================================================
    == Bounds3
    ============================================================
    */
    Bounds3::Bounds3()
        : min(), max() {}
        
    Bounds3::Bounds3(const Bounds3& other)
        : min(other.min), max(other.max) {
    }
    
    Bounds3::~Bounds3() {
    }
    
    Bounds3& Bounds3::operator=(const Bounds3& other) {
        this->min = other.min;
        this->max = other.max;
        return *this;
    }
    
    f32_t Bounds3::width() const {
        return max.x - min.x;
    }
    
    void Bounds3::width(f32_t val) {
        f32_t c = (min.x + max.x) * 0.5f;
        min.x = c - val * 0.5f;
        max.x = c + val * 0.5f;
    }
    
    f32_t Bounds3::height() const {
        return max.y - min.y;
    }
    
    void Bounds3::height(f32_t val) {
        f32_t c = (min.y + max.y) * 0.5f;
        min.y = c - val * 0.5f;
        max.y = c + val * 0.5f;
    }
    
    f32_t Bounds3::depth() const {
        return max.z - min.z;
    }
    
    void Bounds3::depth(f32_t val) {
        f32_t c = (min.z + max.z) * 0.5f;
        min.z = c - val * 0.5f;
        max.z = c + val * 0.5f;
    }
    
    Vector3 Bounds3::size() const {
        return max - min;
    }
    
    Vector3 Bounds3::center() const {
        return (min + max) * 0.5f;
    }
    
    void Bounds3::center(const Vector3& val) {
        Vector3 d = val - this->center();
        min += d;
        max += d;
    }

    Vector3 Bounds3::extent() const {
        return (max - min) * 0.5f;
    }
    
    void Bounds3::extent(const Vector3& val) {
        Vector3 c = this->center();
        min = c - val;
        max = c + val;
    }
    
    bool Bounds3::contains(const Vector3& p) const {
        return Math::between(p, this->min, this->max);
    }
    
    Bounds3& Bounds3::expand(const Vector3& p) {
        if (this->min.x > p.x) this->min.x = p.x;
        if (this->min.y > p.y) this->min.y = p.y;
        if (this->min.z > p.z) this->min.z = p.z;
        if (this->max.x < p.x) this->max.x = p.x;
        if (this->max.y < p.y) this->max.y = p.y;
        if (this->max.z < p.z) this->max.z = p.z;
        return *this;
    }
    
    Bounds3& Bounds3::expand(const Bounds3& b) {
        this->expand(b.min);
        this->expand(b.max);
        return *this;
    }
    
    /*
    ============================================================
    == Math
    ============================================================
    */
    const f32_t Math::EPSILON_4 = 0.0001f;
    const f32_t Math::EPSILON_5 = 0.00001f;
    const f32_t Math::EPSILON_6 = 0.000001f;
    const f32_t Math::PI = 3.1415926535897932f;
    const f32_t Math::PI_MUL_2 = Math::PI * 2;
    const f32_t Math::PI_DIV_2 = Math::PI * 0.5f;
    const f32_t Math::INV_PI = 1.0f / Math::PI;
    const f32_t Math::SQRT_2 = 1.4142135623730950488016887242097f;
    const f32_t Math::SQRT_3 = 1.7320508075688772935274463415059f;
    const f32_t Math::DEGREES_PER_RADIAN = 180.0f / Math::PI;
    const f32_t Math::RADIANS_PER_DEGREE = Math::PI / 180.0f;
    
    void Math::fill(f32_t (& target)[2], f32_t x, f32_t y) {
        target[0] = x;
        target[1] = y;
    }
    
    void Math::fill(f32_t (& target)[3], f32_t x, f32_t y, f32_t z) {
        target[0] = x;
        target[1] = y;
        target[2] = z;
    }
    
    void Math::fill(f32_t (& target)[4], f32_t x, f32_t y, f32_t z, f32_t w) {
        target[0] = x;
        target[1] = y;
        target[2] = z;
        target[3] = w;
    }
    
    f32_t Math::radianToAngle(f32_t radian) {
        return radian * Math::DEGREES_PER_RADIAN;
    }
    
    f32_t Math::angleToRadian(f32_t degree) {
        return degree * Math::RADIANS_PER_DEGREE;
    }
    
    bool Math::between(f32_t x, f32_t a, f32_t b) {
        return (x >= a && x <= b) || (x >= b && x <= a);
    }
    
    bool Math::between(const Vector2& x, const Vector2& a, const Vector2& b) {
        return Math::between(x.x, a.x, b.x) && Math::between(x.y, a.y, b.y);
    }
    
    bool Math::between(const Vector3& x, const Vector3& a, const Vector3& b) {
        return Math::between(x.x, a.x, b.x) && Math::between(x.y, a.y, b.y) && Math::between(x.z, a.z, b.z);
    }
    
    f32_t Math::clamp(f32_t x, f32_t a, f32_t b) {
        f32_t min = a < b ? a : b;
        f32_t max = a > b ? a : b;
        x = x < min ? min : x;
        x = x > max ? max : x;
        return x;
    }
    
    Vector2 Math::clamp(const Vector2& x, const Vector2& a, const Vector2& b) {
        f32_t rx = Math::clamp(x.x, a.x, b.x);
        f32_t ry = Math::clamp(x.y, a.y, b.y);
        return Vector2(rx, ry);
    }
    
    Vector3 Math::clamp(const Vector3& x, const Vector3& a, const Vector3& b) {
        f32_t rx = Math::clamp(x.x, a.x, b.x);
        f32_t ry = Math::clamp(x.y, a.y, b.y);
        f32_t rz = Math::clamp(x.z, a.z, b.z);
        return Vector3(rx, ry, rz);
    }
    
    f32_t Math::lerp(f32_t a, f32_t b, f32_t t) {
        // a * (1-t) + b * t
        return a + (b - a) * t;
    }
    
    Vector2 Math::lerp(const Vector2& a, const Vector2& b, f32_t t) {
        f32_t rx = Math::lerp(a.x, b.x, t);
        f32_t ry = Math::lerp(a.y, b.y, t);
        return Vector2(rx, ry);
    }
    
    Vector3 Math::lerp(const Vector3& a, const Vector3& b, f32_t t) {
        f32_t rx = Math::lerp(a.x, b.x, t);
        f32_t ry = Math::lerp(a.y, b.y, t);
        f32_t rz = Math::lerp(a.z, b.z, t);
        return Vector3(rx, ry, rz);
    }
    
    Vector2 Math::lerp(const Vector2& a, const Vector2& b, const Vector2& t) {
        f32_t rx = Math::lerp(a.x, b.x, t.x);
        f32_t ry = Math::lerp(a.y, b.y, t.y);
        return Vector2(rx, ry);
    }
    
    Vector3 Math::lerp(const Vector3& a, const Vector3& b, const Vector3& t) {
        f32_t rx = Math::lerp(a.x, b.x, t.x);
        f32_t ry = Math::lerp(a.y, b.y, t.y);
        f32_t rz = Math::lerp(a.z, b.z, t.z);
        return Vector3(rx, ry, rz);
    }
    
    f32_t Math::sampleSinCurve(f32_t a, f32_t w, f32_t q, f32_t k, f32_t min, f32_t max, f32_t t) {
        f32_t x = Math::lerp(min, max, t);
        f32_t value = a * sin(w * x + q) + k;
        return value;
    }
    
    f32_t Math::sampleCosCurve(f32_t a, f32_t w, f32_t q, f32_t k, f32_t min, f32_t max, f32_t t) {
        f32_t x = Math::lerp(min, max, t);
        f32_t value = a * cos(w * x + q) + k;
        return value;
    }
    
    // b(t) = (1-t)^2*p0 + 2t(1-t)*p1 + t^2 * p2
    Vector3 Math::sampleBezierCurve(const Vector3 p0, const Vector3 p1, const Vector3& p2, f32_t t) {
        return p0 * (1 - t) * (1 - t) + p1 * 2 * t * (1 - t) + p2 * t * t;
    }
    
    // b(t) = (1-t)^3*p0 + 3t(1-t)^2*p1 + 3t^2(1-t)*p2 + t^3*p3
    Vector3 Math::sampleBezierCurve(const Vector3 p0, const Vector3 p1, const Vector3& p2, const Vector3& p3, f32_t t) {
        return p0 * (1 - t) * (1 - t) * (1 - t) + p1 * 3 * t * (1 - t) * (1 - t) + p2 * 3 * t * t * (1 - t) + p3 * t * t * t;
    }
    
    bool Math::intersects(const Ray& ray, const Plane& plane) {
        f32_t t = FLT_MAX;
        return Math::intersects(t, ray, plane);
    }
    
    bool Math::intersects(const Ray& ray, const Sphere& sphere) {
        f32_t t = FLT_MAX;
        return Math::intersects(t, ray, sphere);
    }
    
    bool Math::intersects(const Ray& ray, const Bounds3& bounds) {
        f32_t t = FLT_MAX;
        return Math::intersects(t, ray, bounds);
    }
    
    /*
        ray : p = p0 + tu
        plane : n*(p - p1) = 0
        t = (n*p1 - n*p0) / (n*u)
    */
    bool Math::intersects(f32_t& t, const Ray& ray, const Plane& plane) {
        f32_t a = Vector3::dot(plane.normal, ray.diBounds2ion);
        if (_FloatEqual(a, 0))
            return false;
        f32_t b = Vector3::dot(plane.normal, plane.normal * plane.distance);
        f32_t c = Vector3::dot(plane.normal, ray.origin);
        t = (b - c) / a;
        return t >= 0;
    }
    
    bool Math::intersects(f32_t& t, const Ray& ray, const Sphere& sphere) {
        Vector3 d = sphere.origin - ray.origin;
        if (d.sqrmagnitude() < sphere.radius * sphere.radius)
            return false;
        Vector3 ao = sphere.origin - ray.origin;
        f32_t hv = Vector3::dot(ao, ray.diBounds2ion);
        Vector3 ap = ray.diBounds2ion * hv;
        f32_t el = (sphere.radius * sphere.radius) - (ao - ap).sqrmagnitude();
        if (el < 0)
            return false;
        t = hv - el;
        return t >= 0;
    }
    
    bool Math::intersects(f32_t& t, const Ray& ray, const Bounds3& bounds) {
        Vector3 min = bounds.min;
        Vector3 max = bounds.max;
        if ((ray.origin.x > min.x || ray.origin.y > min.y || ray.origin.z > min.z) && (ray.origin.x < max.x || ray.origin.y < max.y || ray.origin.z < max.z))
            return false;
        
        if (ray.origin.x <= min.x && ray.diBounds2ion.x <= 0)
            return false;
        if (ray.origin.x >= max.x && ray.diBounds2ion.x >= 0)
            return false;
        
        if (ray.origin.y <= min.y && ray.diBounds2ion.y <= 0)
            return false;
        if (ray.origin.y >= max.y && ray.diBounds2ion.y >= 0)
            return false;
        
        if (ray.origin.z <= min.z && ray.diBounds2ion.z <= 0)
            return false;
        if (ray.origin.z >= max.z && ray.diBounds2ion.z >= 0)
            return false;
        
        Plane planes[] = {Plane(Vector3(-1, 0, 0), bounds.extent().x),    // min x
            Plane(Vector3(1, 0, 0), bounds.extent().x),        // max z
            Plane(Vector3(0, -1, 0), bounds.extent().y),    // min y
            Plane(Vector3(0, 1, 0), bounds.extent().y),        // max y
            Plane(Vector3(0, 0, -1), bounds.extent().z),    // min z
            Plane(Vector3(0, 0, 1), bounds.extent().z)        // max z
        };
        f32_t ts[6] = {FLT_MAX};
        for (int i = 0; i < 6; i++) {
            Math::intersects(ts[i], ray, planes[i]);
        }
        f32_t minT = FLT_MAX;
        for (int i = 0; i < 6; i++) {
            if (ts[i] < minT) minT = ts[i];
        }
        t = minT;
        return t >= 0;
    }
    
    bool Math::intersects(f32_t& t, const Ray& ray, const Vector3& v0, const Vector3& v1, const Vector3 v2) {
        t = 0;
        float u = 0;
        float v = 0;
        
        Vector3 e1 = v1 - v0;
        Vector3 e2 = v2 - v0;
        Vector3 p = Vector3::cross(ray.diBounds2ion, e2);
        
        // determinant
        float det = Vector3::dot(e1, p);
        
        // keep det > 0, modify T accordingly
        Vector3 T;
        if (det > 0) {
            T = ray.origin - v0;
        } else {
            T = v0 - ray.origin;
            det = -det;
        }
        
        // If determinant is near zero, ray lies in plane of triangle
        if (det < 0.0001f)
            return false;
        
        // Q
        Vector3 Q = Vector3::cross(T, e1);
        
        // Calculate u and make sure u <= 1
        u = Vector3::dot(T, p);
        if (u < 0.0f || u > det)
            return false;
        
        // Calculate v and make sure u + v <= 1
        v = Vector3::dot(ray.diBounds2ion, Q);
        if (v < 0.0f || u + v > det)
            return false;
        
        // Calculate t, scale parameters, ray intersects triangle
        t = Vector3::dot(e2, Q);
        
        float invdet = 1.0f / det;
        t *= invdet;
        u *= invdet;
        v *= invdet;
        
        return true;
    }
}
