#include "PlyReader.h"

using namespace godot;

void PlyReader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_ply","variant"), &PlyReader::load_ply);
}

int PlyReader::_type_to_bits(String type) {
	// Convertit la chaîne de type en un entier (8, 16, 32 ou 64).
	if (type == "float" || type == "float32") {
		return -32; // lecture en virgule (pour les vertex)
	} else if (type == "uchar" || type == "char" || type == "int8" || type == "uint8") {
		return 8;
	} else if (type == "short" || type == "int16" || type == "ushort" || type == "uint16") {
		return 16;
	} else if (type == "int" || type == "int32" || type == "uint" || type == "uint32") {
		return 32; // lecture d'entiers (pour les faces)
	} else if (type == "double" || type == "float64") {
		return 64;
	}
	return 0;
}

float PlyReader::read_float_for_type(Ref<FileAccess> file, int type) {
    // Selon le type, lit le bon nombre de bits.
    switch (type) {
        case 8:
            return static_cast<float>(file->get_8());
        case 16:
            return static_cast<float>(file->get_16());
		case 32:
            return static_cast<float>(file->get_32());
        case -32:
            return file->get_float();
        case 64:
            return static_cast<float>(file->get_double());
        default:
            return 0.0f;
    }

}
int32_t PlyReader::read_int_for_type(Ref<FileAccess> file, int type) {
    // Lecture d'un entier selon le type.
    switch (type) {
        case 8:
            return static_cast<int32_t>(file->get_8());
        case 16:
            return static_cast<int32_t>(file->get_16());
        case 32:
            return file->get_32();
        default:
            return 0;
    }
}

PackedInt32Array PlyReader::face_to_indices(const Array &faces) {
	PackedInt32Array indices;
	// Pour chaque face, on crée les triangles (double face pour le culling)
	for (int i = 0; i < faces.size(); i++) {
		Array face = faces[i];
		int face_size = face.size();
		if (face_size == 3) {
			// Triangle simple : on ajoute la face et sa version retournée.
			indices.append(face[0]);
			indices.append(face[1]);
			indices.append(face[2]);
			indices.append(face[2]);
			indices.append(face[1]);
			indices.append(face[0]);
		} else if (face_size > 3) {
			// Triangulation en éventail autour du premier sommet.
			for (int j = 1; j < face_size - 1; j++) {
				indices.append(face[0]);
				indices.append(face[j]);
				indices.append(face[j + 1]);
				// Version inversée pour le backface culling
				indices.append(face[j + 1]);
				indices.append(face[j]);
				indices.append(face[0]);
			}
		}
	}
	return indices;
}

Array PlyReader::data_to_arrays(const PackedVector3Array &vertices,
		const PackedInt32Array &indices,
		const PackedColorArray &colors,
		const PackedVector3Array &normals,
		const PackedVector2Array &uv_mapping) {
	Array arrays;
	arrays.resize(ArrayMesh::ARRAY_MAX);
	arrays[ArrayMesh::ARRAY_VERTEX] = vertices;
	if (!colors.is_empty())
		arrays[ArrayMesh::ARRAY_COLOR] = colors;
	if (!indices.is_empty())
		arrays[ArrayMesh::ARRAY_INDEX] = indices;
	if (!normals.is_empty())
		arrays[ArrayMesh::ARRAY_NORMAL] = normals;
	if (!uv_mapping.is_empty())
		arrays[ArrayMesh::ARRAY_TEX_UV] = uv_mapping;
	return arrays;
}

Array PlyReader::_read_ascii_ply(Ref<FileAccess> file, const Array &property, int vertex_count, int face_count) {
	Array faces;
	PackedVector3Array vertices;
	PackedColorArray colors;
	PackedVector3Array normals;
	PackedVector2Array uv_mapping;

	int property_count = property.size();
	Array values;
	values.resize(property.size());
	Array idxs;
	idxs.resize(12);

	// Détermine quelles propriétés sont présentes.
	bool has_normals = (property.find("nx") != -1);
	bool has_colors = (property.find("red") != -1);
	bool has_alpha = (property.find("alpha") != -1);
	bool has_uvs = (property.find("s") != -1);

	//ont ne sait pas dans quel ordre elle sont ou si elle sont meme presente donc ont recupère l'indice auquel ont l'a recupérer
	idxs[0] = property.find("x");
	idxs[1] = property.find("y");
	idxs[2] = property.find("z");
	if (has_normals) {
		idxs[3] = property.find("nx");
		idxs[4] = property.find("ny");
		idxs[5] = property.find("nz");
	}
	if (has_colors) {
		idxs[6] = property.find("red");
		idxs[7] = property.find("green");
		idxs[8] = property.find("blue");
	}
	if (has_alpha) {
		idxs[9] = property.find("alpha");
	}
	if (has_uvs) {
		idxs[10] = property.find("s");
		idxs[11] = property.find("t");
	}

	// Lecture des sommets (chaque ligne contient les propriétés séparées par des espaces).
	for (int i = 0; i < vertex_count; i++) {
		String line = file->get_line();
		PackedStringArray parts = line.split(" ", false);
		for (int j = 0; j < property_count; j++) {
			values[j] = parts[j].to_float();
		}
		vertices.append(Vector3(values[idxs[0]], values[idxs[1]], values[idxs[2]]));
		if (has_normals) {
			normals.append(Vector3(values[idxs[3]], values[idxs[4]], values[idxs[5]]));
		}
		if (has_colors) {
			if (has_alpha)
				colors.append(Color(values[idxs[6]], values[idxs[7]], values[idxs[8]], values[idxs[9]]));
			else
				colors.append(Color(values[idxs[6]], values[idxs[7]], values[idxs[8]]));
		}
		if (has_uvs) {
			uv_mapping.append(Vector2(values[idxs[10]], values[idxs[11]]));
		}
	}

	// Lecture des faces.
	for (int i = 0; i < face_count; i++) {
		String line = file->get_line();
		Array parts = line.split(" ", false);
		if (parts.size() < 4) {
			print_error("Fichier PLY corrompu (face)", __FUNCTION__, __FILE__, __LINE__);
			return Array();
		}
		int n = static_cast<String>(parts[0]).to_int();
		Array face_indices;
		for (int j = 0; j < n; j++) {
			face_indices.append(static_cast<String>(parts[j + 1]).to_int());
		}
		faces.append(face_indices);
	}
	return data_to_arrays(vertices, face_to_indices(faces), colors, normals, uv_mapping);
}

Array PlyReader::_read_binary_ply(Ref<FileAccess> file, const Array &property, const Array &property_type, const Array &face_type, int vertex_count, int face_count) {
	Array faces;
	PackedVector3Array vertices;
	PackedColorArray colors;
	PackedVector3Array normals;
	PackedVector2Array uv_mapping;

	Array values;
	values.resize(property.size());
	Array idxs;
	idxs.resize(12);

	bool has_normals = (property.find("nx") != -1);
	bool has_colors = (property.find("red") != -1);
	bool has_alpha = (property.find("alpha") != -1);
	bool has_uvs = (property.find("s") != -1);

	idxs[0] = property.find("x");
	idxs[1] = property.find("y");
	idxs[2] = property.find("z");
	if (has_normals) {
		idxs[3] = property.find("nx");
		idxs[4] = property.find("ny");
		idxs[5] = property.find("nz");
	}
	if (has_colors) {
		idxs[6] = property.find("red");
		idxs[7] = property.find("green");
		idxs[8] = property.find("blue");
	}
	if (has_alpha) {
		idxs[9] = property.find("alpha");
	}
	if (has_uvs) {
		idxs[10] = property.find("s");
		idxs[11] = property.find("t");
	}

	// Lecture binaire des sommets.
	for (int i = 0; i < vertex_count; i++) {
		for (int j = 0; j < property.size(); j++) {
			values[j] = this->read_float_for_type(file, static_cast<int>(property_type[j]));
		}
		vertices.append(Vector3(values[idxs[0]], values[idxs[1]], values[idxs[2]]));
		if (has_normals) {
			normals.append(Vector3(values[idxs[3]], values[idxs[4]], values[idxs[5]]));
		}
		if (has_colors) {
			if (has_alpha)
				colors.append(Color(values[idxs[6]], values[idxs[7]], values[idxs[8]], values[idxs[9]]));
			else
				colors.append(Color(values[idxs[6]], values[idxs[7]], values[idxs[8]]));
		}
		if (has_uvs) {
			uv_mapping.append(Vector2(values[idxs[10]], values[idxs[11]]));
		}
	}

	// Lecture binaire des faces.
	for (int i = 0; i < face_count; i++) {
		int n = this->read_int_for_type(file, static_cast<int>(face_type[0]));
		Array face_indices;
		for (int j = 0; j < n; j++) {
			int index = this->read_int_for_type(file, static_cast<int>(face_type[1]));
			face_indices.append(index);
		}
		faces.append(face_indices);
	}
	return this->data_to_arrays(vertices, this->face_to_indices(faces), colors, normals, uv_mapping);
}

MeshInstance3D* PlyReader::create_mesh(const Array &arrays, bool pointCloud) {
	// Création de l'ArrayMesh et ajout de la surface depuis les tableaux.
	ArrayMesh *mesh = memnew(ArrayMesh);
	if (pointCloud) {
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_POINTS, arrays);
	} else {
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	}

	// Création d'un matériel qui utilise les couleurs par sommet.
	StandardMaterial3D *mat = memnew(StandardMaterial3D);
	mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	mesh->surface_set_material(0, mat);

	MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
	mesh_instance->set_mesh(mesh);
	return mesh_instance;
}

MeshInstance3D* PlyReader::load_ply(String file_path, bool quiet) {
	//on utilise le system de pointeur intelligent uniquement sur les class qui ne sont pas des nodes
	Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
	if (file.is_null()) {
		print_error("Erreur lors de l'ouverture du fichier : " + file_path, __FUNCTION__, __FILE__, __LINE__);
		return nullptr;
	}

	// Vérification du header PLY.
	String line = file->get_line().strip_edges();
	if (line != "ply") {
		print_error("Ce fichier n'est pas un fichier PLY valide.", __FUNCTION__, __FILE__, __LINE__);
		return nullptr;
	}

	bool is_ascii = true;
	line = file->get_line().strip_edges();
	if (!line.begins_with("format ascii")) {
		if (line.begins_with("format binary_little_endian 1.0"))
			is_ascii = false;
		else {
			print_error("Format inconnu : " + line, __FUNCTION__, __FILE__, __LINE__);
			return nullptr;
		}
	}

	// Variables d'en-tête.
	int vertex_count = 0;
	int face_count = 0;
	Array property;
	Array property_type;
	Array face_type;

	line = file->get_line().strip_edges();
	while (line != "end_header") {
		if (line.begins_with("element vertex")) {
			vertex_count = line.get_slice(" ", 2).to_int();
		} else if (line.begins_with("element face")) {
			face_count = line.get_slice(" ", 2).to_int();
			// Pour la face, on lit une ligne supplémentaire pour obtenir le type.
			line = file->get_line().strip_edges();
			face_type.append(this->_type_to_bits(line.get_slice(" ", 2)));
			face_type.append(this->_type_to_bits(line.get_slice(" ", 3)));
		} else if (line.begins_with("property") && !line.begins_with("property list")) {
			property.append(line.get_slice(" ", 2));
			if (!is_ascii)
				property_type.append(this->_type_to_bits(line.get_slice(" ", 1)));
		}
		line = file->get_line().strip_edges();
	}

	if (vertex_count == 0 || property.size() < 3) {
		print_error("Aucun sommet (vertex) n'a été trouvé dans le fichier PLY.", __FUNCTION__, __FILE__, __LINE__);
		return nullptr;
	}

	Array arrays;
	if (is_ascii)
		arrays = this->_read_ascii_ply(file, property, vertex_count, face_count);
	else
		arrays = this->_read_binary_ply(file, property, property_type, face_type, vertex_count, face_count);

	if (arrays.size() == 0)
		return nullptr;
	file->close();
	return this->create_mesh(arrays, face_count == 0);
}

