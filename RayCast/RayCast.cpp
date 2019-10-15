
#include <math.h>
#include "RayCast.h"
#include <optional>
#include <variant>

Vec3<double> pix_col(black);

//fonction d'intersection entre une sphere et un rayon de lumiere
std::optional<double> intersect(Sphere S, Ray L) {
	double a = 1/*dot2(L.direction)*/;
	double b = 2 * (dot(L.position, L.direction) - dot(S.position, L.direction));
	double c = dot(L.position, L.position) + dot(S.position, S.position) - (2 * dot(S.position, L.position)) - (S.rayon * S.rayon);

	double delta = (b * b) - (4 * a * c);

	if (delta >= 0) {
		delta = sqrt(delta);
		const double t0 = (-b - delta) / 2;
		const double t1 = (-b + delta) / 2;
		if (t0 >= 0) {
			return t0;
		}
		else if (t1 >= 0)
		{
			return t1;
		}
	}
	else
	{
		return nullopt;
	}
}



optional<double> intersectBox(Nodes n, Ray r) {
	Vec3<double> dir = 1.0 / r.direction;

	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	double t1 = (n.bb.minPos.x - r.position.x) * dir.x;
	double t2 = (n.bb.maxPos.x - r.position.x) * dir.x;
	double t3 = (n.bb.minPos.y - r.position.y) * dir.y;
	double t4 = (n.bb.maxPos.y - r.position.y) * dir.y;
	double t5 = (n.bb.minPos.z - r.position.z) * dir.z;
	double t6 = (n.bb.maxPos.z - r.position.z) * dir.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		return nullopt;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		return nullopt;
	}
	return tmin;
}



BoundingBox createcoordBoundingBox(vector<Sphere> listObjets) {
	double maxX = 0, minX = 0;
	double maxY = 0, minY = 0;
	double maxZ = 0, minZ = 0;

	BoundingBox b;
	for (int i = 0; i < listObjets.size(); i++)
	{
		double maxPosx = listObjets[i].position.x + listObjets[i].rayon;
		double maxPosy = listObjets[i].position.y + listObjets[i].rayon;
		double maxPosz = listObjets[i].position.z + listObjets[i].rayon;

		double minPosx = listObjets[i].position.x - listObjets[i].rayon;
		double minPosy = listObjets[i].position.y - listObjets[i].rayon;
		double minPosz = listObjets[i].position.z - listObjets[i].rayon;

		//Test du max
		if (maxPosx > maxX) {
			maxX = maxPosx;
		}
		if (maxPosy > maxY) {
			maxY = maxPosy;
		}
		if (maxPosz > maxZ) {
			maxZ = maxPosz;
		}
		//test du min 
		if (minPosx < minX) {
			minX = minPosx;
		}
		if (minPosy > minY) {
			minY = minPosy;
		}
		if (minPosz > minZ) {
			minZ = minPosz;
		}
	}
	b.maxPos = { maxX,maxY,maxZ };
	b.minPos = { minX,minY,minZ };

	return b;
}

int splitSphere(vector<Sphere> listObjets) {
	int index = (int)ceil(listObjets.size() / 2);
	return index;
}

variant<Nodes*, Leaf> createBVH(vector<Sphere> listObjetsSplit) {

	// creer les boundingbox pour chaque sphere
	if (listObjetsSplit.size() == 1) {
		Leaf l;
		l.sphere = listObjetsSplit[0];
		return l;
	}
	else {
		int index = splitSphere(listObjetsSplit);

		vector<Sphere> list1;
		for (int i = 0; i < index; i++) {
			list1.push_back(listObjetsSplit[i]);
		}

		vector<Sphere> list2;
		for (int i = index; i < listObjetsSplit.size(); i++) {
			list2.push_back(listObjetsSplit[i]);
		}

		Vec3<double> * bbmax;
		Vec3<double> * bbmin;

		BoundingBox b = createcoordBoundingBox(list1);

		variant<Nodes*, Leaf> first = createBVH(list1);
		variant<Nodes*, Leaf> second = createBVH(list2);

		return new Nodes(first, second,b);
	}
}


Vec3<double> RayCast(Ray ray,int nbRebonds,variant<Nodes*,Leaf> BVH) {
	Vec3<double> couleur = black;
	Vec3<double> pix_col = black;
	int rb = nbRebonds + 1;
	//calcul du t_min entre les spheres pour savoir s'il y en a une devant l'autre de la camera
	int index = 0;
	std::optional<double> t_min = nullopt;
	for (int i = 0; i < objetsScenes.size(); i++) {
		std::optional<double> t = intersect(objetsScenes[i], ray);
		if ((!t_min.has_value() && t.has_value()) || (t.has_value() && t.value() < t_min.value())) {
			index = i;
			indexAlbedo = i;
			t_min = t;
		}
	}

	if (t_min.has_value()) {
		//sphere miroir
		Vec3<double> posIntersection = ray.position + (ray.direction * (t_min.value() - 0.1));
		if (objetsScenes[index].albedo >= 1) {
			
			Vec3<double> normalIntersection = posIntersection - objetsScenes[index].position;
			Vec3<double> rebondMiroir = normalIntersection * 2 * dot(ray.direction * (-1), normalize(normalIntersection)) + ray.direction;

			Ray rebond{ posIntersection,
						rebondMiroir };

			return RayCast(rebond,rb,BVH);
		}

		Vec3<double> normalIntersection = normalize(posIntersection - objetsScenes[index].position);

		Vec3<double> randomdir{ random_between_two_float(-0.0001,0.0001),random_between_two_float(-0.0001, 0.0001),random_between_two_float(-0.0001, 0.0001) };
		Vec3<double> rebondMiroir = normalIntersection * 2 * dot(ray.direction * (-1), normalize(normalIntersection)) + ray.direction;
		Ray rebondLumiereIndirect = { posIntersection,
								      normalize(rebondMiroir ) };

		for (int l = 0; l < LumieresScenes.size(); l++) {
			for (int h = 0; h < nbLumieresSurface; h++) {

				//generation d'une lumiere aleatoire pour les lumieres surfaciques
				int randX = rand() % 20;
				int randY = rand() % 19;
				int randZ = rand() % 19;
				Lumiere lightSu{ Vec3<double>{LumieresScenes[l].position.x + randX,
											  LumieresScenes[l].position.y + randY,
											  LumieresScenes[l].position.z + randZ}, white, 5 };

				Vec3<double> DirectionLumiere = lightSu.position - posIntersection;
				double dt = abs(dot(normalize(DirectionLumiere), normalIntersection));

				//ajout de 0.1 pour pas que la sphere se teste elle-même
				Ray rebond{
					posIntersection + (normalize(DirectionLumiere)),
					normalize(DirectionLumiere)
				};

				//test s'il y a une sphere entre la sphere testée et la lumiere
				std::optional<double> rebondIntersecte_min = nullopt;
				for (int i = 0; i < objetsScenes.size(); i++) {
					std::optional<double> t = intersect(objetsScenes[i], rebond);
					if ((!rebondIntersecte_min.has_value()) || (t.has_value() && t.value() < rebondIntersecte_min.value())) {
						rebondIntersecte_min = t;
					}
				}

				if (!rebondIntersecte_min.has_value()) {
					couleur = couleur + objetsScenes[index].couleur * lightSu.couleur * dt / (norm(DirectionLumiere) * LumieresScenes.size() * nbLumieresSurface);
				}
				//test si une sphere a intersecter un objet derriere la lumiere 
				else if (rebondIntersecte_min.value() >= norm(DirectionLumiere))
				{
					couleur = couleur + objetsScenes[index].couleur * lightSu.couleur * dt / (norm(DirectionLumiere) * LumieresScenes.size() * nbLumieresSurface);
				}
				else
				{
					couleur = black + couleur;
				}
			}
		}
		if (rb < rebondMax) {
			pix_col = (couleur * (1 - objetsScenes[index].albedo)) + (RayCast(rebondLumiereIndirect, rb, BVH) * objetsScenes[index].albedo);
		}
	}

	return pix_col;
}

int main() {
	//en tête du fichier ppm
	std::ofstream out("out.ppm");
	out << "P3\n" << W << ' ' << H << ' ' << "255\n";

	//----------------------ajout des spheres----------------------
	objetsScenes.push_back(sphere);
	objetsScenes.push_back(sphere1);
	objetsScenes.push_back(sphere2);
	objetsScenes.push_back(spherefond);
	objetsScenes.push_back(spheresol);
	objetsScenes.push_back(sphereplafond);
	objetsScenes.push_back(sphereDroit);
	objetsScenes.push_back(spheregauche);
	objetsScenes.push_back(spheredevant);

	//----------------------ajout des Lumieres----------------------
	//LumieresScenes.push_back(light);
	LumieresScenes.push_back(posLumierSurfacique);
	LumieresScenes.push_back(posLumierSurfacique);
	LumieresScenes.push_back(posLumierSurfacique2);
	LumieresScenes.push_back(posLumierSurfacique3);
	LumieresScenes.push_back(posLumierSurfacique3);
	LumieresScenes.push_back(posLumierSurfacique3);

	//----------------------Debut du dessin-------------------------
	//https://en.cppreference.com/w/cpp/utility/variant

	std::variant<Nodes*,Leaf> final = createBVH(objetsScenes);

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {

			pix_col = black;
			Vec3<double> directionCamera = normalize(Vec3<double>{ (double)x, (double)y, 0 } -positionPerspective);

			Ray ray{
				Vec3<double>{(double)x, (double)y, 0},
				directionCamera
			};

			Vec3<double> col = RayCast(ray, 0, final);

			clamp255(col);
			out << (int)col.x << ' '
				<< (int)col.y << ' '
				<< (int)col.z << '\n';
		}
	}
}