#pragma once

#include "ray.h"
#include "vec3Utils.h"
#include <vector>

using namespace std;

class material;

struct intersection {
	vec3 point;
	vec3 norm;
	double dist;
	bool front_face;
	material* mat;

	float t, u, v; //instance variables for rectangle/cube objects

	inline void set_face_normal(const ray& r, const vec3& outward_normal) {
		front_face = dot(r.getDirection(), outward_normal) < 0;
		norm = front_face ? outward_normal : -outward_normal;
	};
};


class material {
public:
	virtual bool scatter(const ray& r, const intersection& intrsct, vec3& attenuation, ray& scattered) {
		return 0;
	}
};

class lambertian : public material {
public:
	vec3 color;
	lambertian(const vec3& c) {
		color = c;
	}

	virtual bool scatter(const ray& r, const intersection& intrsct, vec3& attenuation, ray& scattered) override {
		auto dir = intrsct.norm + randomUnitVec3();

		if (nearZero(dir)) {
			dir = intrsct.norm;
		}

		scattered = ray(intrsct.point, dir);
		attenuation = color;
		return true;
	}
};

class metal : public material {
public:
	vec3 color;
	metal(const vec3& c) {
		color = c;
	}

	virtual bool scatter(const ray& r, const intersection& intrsct, vec3& attenuation, ray& scattered) override {
		vec3 reflected = reflect(normalize(r.getDirection()), intrsct.norm);
		scattered = ray(intrsct.point, reflected);
		attenuation = color;
		return (dot(scattered.getDirection(), intrsct.norm) > 0);
	}
};

class blurryMetal : public material {
public:
	vec3 color;
	float blur;
	blurryMetal(const vec3& c, float b) {
		color = c;
		blur = b;
	}

	virtual bool scatter(const ray& r, const intersection& intrsct, vec3& attenuation, ray& scattered) override {
		vec3 reflected = reflect(normalize(r.getDirection()), intrsct.norm);
		scattered = ray(intrsct.point, reflected + blur * randomUnitVec3());
		attenuation = color;
		return (dot(scattered.getDirection(), intrsct.norm) > 0);
	}
};

class dielectric : public material {//New material class
public:
	double ir;
	dielectric(double index) {
		ir = index;
	}
	virtual bool scatter(const ray& r, const intersection& intrsct, vec3& attenuation, ray& scattered) override {
		attenuation = vec3(1.0, 1.0, 1.0);
		double refractRatio = intrsct.front_face ? (1.0 / ir) : ir;
		
		vec3 unitDir = normalize(r.getDirection());
		double cos = fmin(dot(-unitDir, intrsct.norm), 1.0);
		double sin = sqrt(1.0 - cos * cos);

		vec3 direction;

		if ((refractRatio * sin > 1.0) || reflectance(cos, refractRatio) > random_double())
			direction = reflect(unitDir, intrsct.norm);
		else
			direction = refract(unitDir, intrsct.norm, refractRatio);

		scattered = ray(intrsct.point, direction);
		return true;
	}
	double reflectance(float cos, float inx)//Schlicks approximation for reflectance
	{
		double r0 = (1 - inx) / (1 + inx);
		r0 = r0 * r0;
		return r0 + (1 - r0) * pow((1 - cos), 5);
	}
};

class lightEmitting : public material {
public:
	vec3 lightColor;
	lightEmitting(vec3 c) {
		lightColor = c;
	}

	virtual bool scatter(const ray& r, const intersection& intrsct, vec3& attenuation, ray& scattered) override {		
		attenuation = lightColor;
		return false;
	}
};

class object {
public:
	vec3 centre;
	material* mat;
	virtual bool intersect(ray& r, double t_min, double t_max, intersection& intersect) {
		return 0;
	}
};

class sphere : public object {
public:	
	float radius;
	sphere() { radius = 0; }
	sphere(vec3 cntr, float rad, material* m) {
		centre = cntr;
		radius = rad;
		mat = m;
	}
	bool intersect(ray& r, double minDist, double maxDist, intersection& intersect) {
		vec3 dir = r.getOrigin() - centre;
		float a = dot(r.getDirection(), r.getDirection());
		float b = 2.0 * dot(dir, r.getDirection());
		float c = dot(dir, dir) - radius * radius;
		auto disc = b * b - 4 * a * c;
		if (disc < 0) {
			return false;
		}
		float root = (-b - sqrt(disc)) / (2.0 * a);
		if (root < minDist || maxDist < root) {
			root = (-b + sqrt(disc)) / (2.0 * a);
			if (root < minDist || maxDist < root)
				return false;
		}

		intersect.dist = root;
		intersect.point = r.at(root);
		intersect.mat = mat;
		vec3 normal = (intersect.point - centre) / radius;
		intersect.set_face_normal(r, normal);
		return true;
	}
};

class polygon : public object {
public:
	vec3 point0, point1, point2, pn;
	float d;
	polygon() { }
	polygon(vec3 p0, vec3 p1, vec3 p2, material* m) {
		point0 = p0;
		point1 = p1;
		point2 = p2;
		centre = (point0 + point1 + point2) / 3;
		mat = m;

		vec3 vecA, vecB;
		vecA = point1 - point0;//p0p1
		vecB = point2 - point0;//p0p2
		pn = cross(vecA, vecB);
		d = (-1) * ((pn[0] * point0[0]) + (pn[1] * point0[1]) + (pn[2] * point0[2]));
	}
	bool intersect(ray& r, double minDist, double maxDist, intersection& intersect) {
		//intersects with plane of triangle
		if (fabs(dot(pn, r.getDirection())) < 0.0001)
			return false;

		float t = -1 * ((dot(pn, r.getOrigin()) + d) / (dot(pn, r.getDirection())));		
		if (t < 0) {
			return false;
		}

		//point of intersection
		vec3 P = r.at(t);

		//interior test of triangle
		vec3 lineA, lineB, lineC;
		float distA, distB, distC;
		lineA = point0 - point1;//p0p1
		lineB = point1 - point2;//p1p2
		lineC = point2 - point0;//p2p0
		vec3 C0, C1, C2;
		C0 = P - point0;
		C1 = P - point1;
		C2 = P - point2;

		if ((dot(pn, cross(lineA, C0)) >= 0 &&
			dot(pn, cross(lineB, C1)) >= 0 &&
			dot(pn, cross(lineC, C2)) >= 0) || 
			(dot(pn, cross(lineA, C0)) <= 0 &&
			dot(pn, cross(lineB, C1)) <= 0 &&
			dot(pn, cross(lineC, C2)) <= 0)) {

			vec3 dvec = P - r.getOrigin();
			float dist = sqrt(dot(dvec, dvec));
			if (dist < minDist || maxDist < dist)
				return false;

			// P is inside the triangle
			intersect.dist = dist;
			intersect.point = P;
			intersect.mat = mat;
			vec3 normal = pn;
			intersect.set_face_normal(r, normal);
			return true; 
		}		
		return false;
	}
};

class objectList {
public:
	vector<object*> objects;
	objectList() {}
	objectList(vector<object*> objs) {
		objects = objs;
	}
	void clear() { objects.clear(); }
	void add(object* obj) {
		objects.push_back(obj);
	}
	bool findFirstIntersection(ray& r, double minDist, double maxDist, intersection& intersect) {
		intersection temp;
		bool anyIntersection = false;
		auto closest_so_far = maxDist;

		for (object* object : objects) {
			if (object->intersect(r, minDist, closest_so_far, temp)) {
				anyIntersection = true;
				closest_so_far = temp.dist;
				intersect = temp;
			}
		}

		return anyIntersection;

	}
};

class rectangle : public object {
public:
	vec3 point0, point1, point2, point3, pn;
	float d;

	rectangle(const vec3 p0, const vec3 p1, const vec3 p2, const vec3 p3, material* m) {
		point0 = p0;
		point1 = p1;
		point2 = p2;
		point3 = p3;
		centre = (point0 + point1 + point2 + point3) / 3;

		mat = m;

		vec3 vecA, vecB;
		vecA = point1 - point0;//p0p1
		vecB = point2 - point0;//p0p2
		pn = cross(vecA, vecB);
		d = (-1) * ((pn[0] * point0[0]) + (pn[1] * point0[1]) + (pn[2] * point0[2]));
	}

	bool intersect(ray& r, double minDist, double maxDist, intersection& intrsct) {
		//intersects with plane
		if (fabs(dot(pn, r.getDirection())) < 0.0001)
			return false;

		float t = -1 * ((dot(pn, r.getOrigin()) + d) / (dot(pn, r.getDirection())));
		if (t < 0) {
			return false;
		}

		//point of intersection
		vec3 P = r.at(t);

		//interior test
		vec3 lineA, lineB, lineC, lineD;
		float distA, distB, distC, distD;
		lineA = point0 - point1;//p0p1
		lineB = point1 - point2;//p1p2
		lineC = point2 - point3;//p2p3
		lineD = point3 - point0;//p3p0
		vec3 C0, C1, C2, C3;
		C0 = P - point0;
		C1 = P - point1;
		C2 = P - point2;
		C3 = P - point3;

		if ((dot(pn, cross(lineA, C0)) >= 0 &&
			dot(pn, cross(lineB, C1)) >= 0 &&
			dot(pn, cross(lineC, C2)) >= 0 &&
			dot(pn, cross(lineD, C3)) >= 0) ||
			(dot(pn, cross(lineA, C0)) <= 0 &&
				dot(pn, cross(lineB, C1)) <= 0 &&
				dot(pn, cross(lineC, C2)) <= 0 &&
				dot(pn, cross(lineD, C3)) <= 0)) {

			vec3 dvec = P - r.getOrigin();
			float dist = sqrt(dot(dvec, dvec));
			if (dist < minDist || maxDist < dist)
				return false;

			// P is inside the triangle
			intrsct.dist = dist;
			intrsct.point = P;
			intrsct.mat = mat;
			vec3 normal = pn;
			intrsct.set_face_normal(r, normal);
			return true;
		}
		return false;
	}
};


class cube : public object {
public:
	vec3 cntr;
	float width, height, depth;
	objectList sideList;
	cube(const vec3 c, const float& w, const float& h, const float& d, material* m) {
		cntr = c;
		mat = m;
		width = w;
		height = h;
		depth = d;		

		//front face +Z
		vec3 p0, p1, p2, p3;
		p0 = c;
		p1 = c;
		p2 = c;
		p3 = c;
		p0.x = p0.x - width / 2;
		p0.y = p0.y + height / 2;
		p0.z = p0.z + depth / 2;

		p1.x = p1.x + width / 2;
		p1.y = p1.y + height / 2;
		p1.z = p1.z + depth / 2;

		p2.x = p2.x + width / 2;
		p2.y = p2.y - height / 2;
		p2.z = p2.z + depth / 2;

		p3.x = p3.x - width / 2;
		p3.y = p3.y - height / 2;
		p3.z = p3.z + depth / 2;

		sideList.add(new rectangle(p3, p2, p1, p0, m));

		//back face -Z
		p0 = c;
		p1 = c;
		p2 = c;
		p3 = c;
		p0.x = p0.x + width / 2;
		p0.y = p0.y + height / 2;
		p0.z = p0.z - depth / 2;

		p1.x = p1.x - width / 2;
		p1.y = p1.y + height / 2;
		p1.z = p1.z - depth / 2;

		p2.x = p2.x - width / 2;
		p2.y = p2.y - height / 2;
		p2.z = p2.z - depth / 2;

		p3.x = p3.x + width / 2;
		p3.y = p3.y - height / 2;
		p3.z = p3.z - depth / 2;		

		sideList.add(new rectangle(p3, p2, p1, p0, m));

		//left face -X
		p0 = c;
		p1 = c;
		p2 = c;
		p3 = c;
		p0.x = p0.x - width / 2;
		p0.y = p0.y + height / 2;
		p0.z = p0.z - depth / 2;

		p1.x = p1.x - width / 2;
		p1.y = p1.y + height / 2;
		p1.z = p1.z + depth / 2;		

		p2.x = p2.x - width / 2;
		p2.y = p2.y - height / 2;
		p2.z = p2.z + depth / 2;

		p3.x = p3.x - width / 2;
		p3.y = p3.y - height / 2;
		p3.z = p3.z - depth / 2;

		sideList.add(new rectangle(p3, p2, p1, p0, m));

		//right face +X
		p0 = c;
		p1 = c;
		p2 = c;
		p3 = c;
		p0.x = p0.x + width / 2;
		p0.y = p0.y + height / 2;
		p0.z = p0.z + depth / 2;

		p1.x = p1.x + width / 2;
		p1.y = p1.y + height / 2;
		p1.z = p1.z - depth / 2;

		p2.x = p2.x + width / 2;
		p2.y = p2.y - height / 2;
		p2.z = p2.z - depth / 2;

		p3.x = p3.x + width / 2;
		p3.y = p3.y - height / 2;
		p3.z = p3.z + depth / 2;		

		sideList.add(new rectangle(p3, p2, p1, p0, m));

		//top face +Y
		p0 = c;
		p1 = c;
		p2 = c;
		p3 = c;
		p0.x = p0.x - width / 2;
		p0.y = p0.y + height / 2;
		p0.z = p0.z - depth / 2;

		p1.x = p1.x + width / 2;
		p1.y = p1.y + height / 2;
		p1.z = p1.z - depth / 2;

		p2.x = p2.x + width / 2;
		p2.y = p2.y + height / 2;
		p2.z = p2.z + depth / 2;

		p3.x = p3.x - width / 2;
		p3.y = p3.y + height / 2;
		p3.z = p3.z + depth / 2;

		sideList.add(new rectangle(p3, p2, p1, p0, m));

		//bottom face -Y
		p0 = c;
		p1 = c;
		p2 = c;
		p3 = c;
		p1.x = p1.x - width / 2;
		p1.y = p1.y - height / 2;
		p1.z = p1.z + depth / 2;

		p2.x = p2.x + width / 2;
		p2.y = p2.y - height / 2;
		p2.z = p2.z + depth / 2;

		p3.x = p3.x + width / 2;
		p3.y = p3.y - height / 2;
		p3.z = p3.z - depth / 2;

		p0.x = p0.x - width / 2;
		p0.y = p0.y - height / 2;
		p0.z = p0.z - depth / 2;

		sideList.add(new rectangle(p3, p2, p1, p0, m));
	}
	bool intersect(ray& r, double minDist, double maxDist, intersection& intersect) {
		return sideList.findFirstIntersection(r, minDist, maxDist, intersect);
	}
};

