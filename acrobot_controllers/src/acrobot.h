#pragma once
#include <iostream>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Core>

#ifndef GRAVITY
#define GRAVITY 9.81
#endif

#ifndef PI
#define PI 3.141592
#endif



using namespace Eigen;
struct Link
{
	float m;
	float l;
	float lc;
	float Ic;
	float b;
	float I;

	Link() {}
	Link(float _m, float _l, float _lc, float _Ic, float _b)
		: m(_m), l(_l), lc(_lc), Ic(_Ic), b(_b), I(_Ic+_m*_lc*_lc) {}
};

class Acrobot
{
private: // variables
	Link link1;
	Link link2;
		
	Vector2f vecDesQ;
	Vector2f vecDesQdot;
	Vector2f vecDesQddot;

	// for Swing up
	Matrix2f matH; // 2x2 matrix
	Matrix2f matC; // 2x2 matrix

	Vector2f vecC; // 2x2 matrix
	Vector2f vecG; // 2 column vector
	Vector2f vecB; // 2 column vector
	Vector2f vecP; // 2 column vector

	float alpha;
	float kp;
	float kd;

	// for LQR
	RowVector4f vecK_lqr; // 4 row vector

private: // functions
	void calcManipulatorDynamics(Vector2f vecQ, Vector2f vecQdot)
	{
		float q1 = vecQ(0);
		float q2 = vecQ(1);
		float q1_dot = vecQdot(0);
		float q2_dot = vecQdot(1);

		//   H(q)q_ddot + C(q,q_dot)q_dot + G(q) = Bu

		// calc matrix H
		matH(0, 0) = link1.I + link2.I + link2.m * link1.l * link1.l + 2.0f * link2.m * link1.l * link2.lc * cos(q2);
		matH(0, 1) = link2.I + link2.m * link1.l * link2.lc * cos(q2);
		matH(1, 0) = matH(0, 1);
		matH(1, 1) = link2.I;

		// calc matrix C
		matC(0, 0) = -2.0f * link2.m * link1.l * link2.lc * sin(q2) * q2_dot;
		matC(0, 1) = -1.0f * link2.m * link1.l * link2.lc * sin(q2) * q2_dot;
		matC(1, 0) = link2.m * link1.l * link2.lc * sin(q2) * q1_dot;
		matC(1, 1) = 0.f;

		// calc vector G
		vecG(0) = GRAVITY * (link1.m * link1.lc * sin(q1) + link2.m * (link1.l * sin(q2) + link2.lc * sin(q1 + q2)));
		vecG(1) = GRAVITY * (link2.m * link2.lc * sin(q1 + q2));

		// calc vector C ??
		// C = C_.dot(qd) + G + array([[b1], [b2]])*qd
		vecC = matC*vecQdot + vecG + 0.1*vecQdot;

		// calc vector B
		vecB(0) = 0;
		vecB(1) = 1;
	
	}

	bool isSmallError(Vector2f vecQ, Vector2f vecQdot, Vector2f vecDesQ, Vector2f vecDesQdot)
	{
		return false;
	}

	float calcErrorState(float qd, float q)
	{
		float dq = fmod(qd, 2*PI) - fmod(q, 2*PI);

		if (dq > PI)
			dq = dq - 2*PI;
		else if (dq <= -PI)
			dq = dq + 2*PI;
		
		return dq;
	}
public: // functions
	Acrobot(Link _link1, Link _link2) : link1(_link1), link2(_link2) {}
	~Acrobot(){}

	void setTargetValue(Vector2f _vecDesQ, Vector2f _vecDesQdot, Vector2f _vecDesQddot)
	{
		vecDesQ << 0, 0;
		vecDesQdot << 0, 0;
		vecDesQddot << 0, 0;
	}

	float calcControlInput(Vector2f _vecQ, Vector2f _vecQdot)
	{
		float fControlInput = 0;
		fControlInput = doSwingUp(_vecQ, _vecQdot, vecDesQ, vecDesQdot, vecDesQddot);

		// if (true == isSmallError(_vecQ, _vecQdot, vecDesQ, vecDesQdot)) {
		// 	// do lqr
		// 	fControlInput = doLQR(_vecQdot, _vecQdot);
		// }
		// else {
		// 	// do swing up
		// 	fControlInput = doSwingUp(_vecQdot, _vecQdot, vecDesQ, vecDesQdot, vecDesQddot);
		// }

		// std::cout << "no controller" << std::endl;
		return fControlInput;
	}

	void setParameter4SwingUp(float fAlpha, float fKp, float fKd)
	{
		alpha = fAlpha;
		kp = fKp;
		kd = fKd;
	}

	float doSwingUp(Vector2f vecQ, Vector2f vecQdot, Vector2f vecDesQ, Vector2f vecDesQdot, Vector2f vecDesQddot)
	{
		calcManipulatorDynamics(vecQ, vecQdot);

		float H22_bar = matH(1, 1) - matH(1, 0) / matH(0, 0) * matH(1, 0);
		if (matH(0, 0) == 0) { H22_bar = 0; }

		float C2_bar = vecC(1) - matH(1, 1) / matH(0, 0) * vecC(0);
		if (matH(0, 0) == 0 || vecC(0) == 0) { C2_bar = 0; }

		float des_q2 = 2 * alpha / PI * atan(vecQdot(0));
						
		float v2 = vecDesQddot(1) + kd * (vecDesQdot(1) - vecQdot(1)) + kp* calcErrorState(des_q2, vecQ(1));

		// Bu = H(q)q_ddot + C(q,q_dot)q_dot + G(q)
		float u = H22_bar*v2 + C2_bar;
		
		return u;
	}
	
	float doLQR(Vector2f vecQ, Vector2f vecQdot)
	{
		Vector4f vecX;
		vecX << vecQ, vecQdot;
		return -vecK_lqr * vecX;

	}

	void setParameter4LQR(float k1, float k2, float k3, float k4)
	{
		vecK_lqr << k1, k2, k3, k4;
	}

};