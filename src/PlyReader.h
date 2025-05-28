#ifndef PLY_READER_H
#define PLY_READER_H

// Les includes de la partie "core"
#include <godot_cpp/core/class_db.hpp>

// Les includes des classes de l'API dans le dossier "classes"
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
using namespace godot;

class PlyReader : public RefCounted {
	GDCLASS(PlyReader, RefCounted);

protected:
	static void _bind_methods();

public:
	// Charge un fichier PLY et retourne un MeshInstance3D généré
	MeshInstance3D* load_ply(String file_path, bool quiet = false);

private:
	// Convertit un tableau (d'Array) de faces en un PackedInt32Array d'indices.
	PackedInt32Array face_to_indices(const Array &faces);

	// Construit le tableau final servant à créer l'ArrayMesh.
	Array data_to_arrays(const PackedVector3Array &vertices,
			const PackedInt32Array &indices,
			const PackedColorArray &colors,
			const PackedVector3Array &normals,
			const PackedVector2Array &uv_mapping);

	Array _read_ascii_ply(Ref<FileAccess> file, const Array &property, int vertex_count, int face_count);

	Array _read_binary_ply(Ref<FileAccess> file, const Array &property, const Array &property_type, const Array &face_type, int vertex_count, int face_count);

	// Retourne un entier décrivant le type (8,16,32,-32(float) ou 64 bits) d’après la chaîne passée.
	int _type_to_bits(String type);

	// Pour un lire float depuis binaire selon le bon nombre d'octet
	float read_float_for_type(Ref<FileAccess> file, int type);

	// Crée et retourne un MeshInstance3D à partir des données (arrays)
	MeshInstance3D *create_mesh(const Array &arrays, bool pointCloud);
	
	//pour lire les faces sui sont forcement en int32
	int32_t PlyReader::read_int_for_type(Ref<FileAccess> file, int type);
};


#endif 
