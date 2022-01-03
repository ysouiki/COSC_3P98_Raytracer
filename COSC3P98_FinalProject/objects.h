#pragma once

#include "ray.h"
#include <vector>

using namespace std;

class material;

struct intersection {
	vec3 point;
	vec3 norm;
	double dist;
	bool front_face;
	material* mat;

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
		vec3 reflected = reflect(normalize(r.getDirection()), intrsct.norm) + blur * randomUnitVec3();
		scattered = ray(intrsct.point, reflected);
		attenuation = color;
		return (dot(scattered.getDirection(), intrsct.norm) > 0);
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
	sphere() {}
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