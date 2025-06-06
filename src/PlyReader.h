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
	ArrayMesh* load_ply(const String& )const;
	// Crée et retourne un MeshInstance3D à partir des données (arrays)
	ArrayMesh* create_mesh(const Array &, bool )const;

	// Construit le tableau final servant à créer l'ArrayMesh.
	Array data_to_arrays(const PackedVector3Array &,
			const PackedInt32Array &,
			const PackedColorArray &,
			const PackedVector3Array &,
			const PackedVector2Array &)const;
private:
	// Convertit un tableau (d'Array) de faces en un PackedInt32Array d'indices.
	PackedInt32Array face_to_indices(const PackedInt32Array&)const;

	void _read_ascii_vertices(Ref<FileAccess> ,float * ,int )const;

	void _read_binary_vertices(Ref<FileAccess>,const PackedInt32Array& ,float * ,int  )const;

	void _read_ascii_faces(Ref<FileAccess> ,PackedInt32Array&,int )const;

	void _read_binary_faces(Ref<FileAccess> ,PackedInt32Array&,const int*,int)const;

	Array _read_ply(Ref<FileAccess> , const Array &, const PackedInt32Array &,const int *, int , int ,bool )const;

	// Retourne un entier décrivant le type (8,16,32,-32(float) ou 64 bits) d’après la chaîne passée.
	int _type_to_bits(String )const;

	// Pour un lire float depuis binaire selon le bon nombre d'octet
	float read_float_for_type(Ref<FileAccess> , int )const;
	
	//pour lire les faces sui sont forcement en int32
	int32_t read_int_for_type(Ref<FileAccess> , int )const;
};


#endif 
