#include "PlyReader.h"

using namespace godot;

void PlyReader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_ply","variant"), &PlyReader::load_ply);
	ClassDB::bind_method(D_METHOD("create_mesh","variant"), &PlyReader::create_mesh);
	ClassDB::bind_method(D_METHOD("data_to_arrays","variant"), &PlyReader::data_to_arrays);
}

int PlyReader::_type_to_bits(String type) const{
	// Convertit la chaîne de type en un entier (8, 16, 32 ou 64).
	if (type == "float" || type == "float32") {
		return -32; // lecture en float (pour les vertex)
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

float PlyReader::read_float_for_type(Ref<FileAccess> file, int type)const {
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
int32_t PlyReader::read_int_for_type(Ref<FileAccess> file, int type)const {
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

PackedInt32Array PlyReader::face_to_indices(const PackedInt32Array &faces) const{
	PackedInt32Array indices;
	// Pour chaque face, on crée les triangles (double face pour le culling)
	int i = 0;
	while(i < faces.size()){
		//nombre de sommet de la face
		int n = faces[i];
		//position du premier sommet = i+1, dernier = i+n
		if (n == 3) {
			// Triangle simple : on ajoute la face et sa version retournée.
			indices.append(faces[i+1]);
			indices.append(faces[i+2]);
			indices.append(faces[i+3]);
			indices.append(faces[i+3]);
			indices.append(faces[i+2]);
			indices.append(faces[i+1]);
		} else if (n > 3) {
			// Triangulation en éventail autour du premier sommet.
			for (int j = 2; j < n; j++) {
				indices.append(faces[i+1]);
				indices.append(faces[i+j]);
				indices.append(faces[i+j+1]);
				// Version inversée pour le backface culling
				indices.append(faces[i+j+1]);
				indices.append(faces[i+j]);
				indices.append(faces[i+1]);
			}
		}
		i = i+n+1;//prochaine face
	}
	return indices;
}

Array PlyReader::data_to_arrays(const PackedVector3Array &vertices,
		const PackedInt32Array &indices,
		const PackedColorArray &colors,
		const PackedVector3Array &normals,
		const PackedVector2Array &uv_mapping)const {
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	if (!colors.is_empty())
		arrays[Mesh::ARRAY_COLOR] = colors;
	if (!indices.is_empty())
		arrays[Mesh::ARRAY_INDEX] = indices;
	if (!normals.is_empty())
		arrays[Mesh::ARRAY_NORMAL] = normals;
	if (!uv_mapping.is_empty())
		arrays[Mesh::ARRAY_TEX_UV] = uv_mapping;
	return arrays;
}

void PlyReader::_read_ascii_vertices(Ref<FileAccess> file,float * values,int taille)const {
	String line = file->get_line();
	PackedStringArray parts = line.split(" ", false);
	for (int j = 0; j < taille; j++)
		values[j] = parts[j].to_float();
}

void PlyReader::_read_binary_vertices(Ref<FileAccess> file, const PackedInt32Array& property_type, float * values, int taille)const {
	for (int j = 0; j < taille; j++) 
		values[j] = this->read_float_for_type(file, property_type[j]);
}

void PlyReader::_read_ascii_faces(Ref<FileAccess> file,PackedInt32Array& faces,int face_count)const{
	for (int i = 0; i < face_count; i++) {
		String line = file->get_line();
		PackedStringArray parts = line.split(" ", false);
		for (int j = 0; j < parts.size(); j++)
			faces.append(parts[j].to_int());
	}
}

void PlyReader::_read_binary_faces(Ref<FileAccess> file,PackedInt32Array& faces,const int* face_types,int face_count)const{
	for (int i = 0; i < face_count; i++) {
		int n = this->read_int_for_type(file, face_types[0]);
		faces.append(n);
		for (int j = 0; j < n; j++) {
			faces.append(this->read_int_for_type(file, face_types[1]));
		}
	}
}

Array PlyReader::_read_ply(Ref<FileAccess> file, const Array &property, const PackedInt32Array &property_type,const int * face_types, int vertex_count, int face_count,bool is_ascii)const {
	PackedInt32Array faces;
	PackedVector3Array vertices;
	vertices.resize(vertex_count);
	PackedColorArray colors;
	colors.resize(vertex_count);
	PackedVector3Array normals;
	PackedVector2Array uv_mapping;
	int taille = property.size();
	float * values = new float[taille];
	int idxs[12];

	// Détermine quelles propriétés sont présentes.
	bool has_normals = (property.find("nx") != -1);
	bool has_colors = (property.find("red") != -1);
	bool has_alpha = (property.find("alpha") != -1);
	bool has_uvs = (property.find("s") != -1);

	//ont ne sait pas dans quel ordre elle sont ou si elle sont meme presente donc ont recupère l'indice auquel ils sont dans le header
	idxs[0] = property.find("x");
	idxs[1] = property.find("y");
	idxs[2] = property.find("z");
	if (has_normals) {
		idxs[3] = property.find("nx");
		idxs[4] = property.find("ny");
		idxs[5] = property.find("nz");
		normals.resize(vertex_count);
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
		uv_mapping.resize(vertex_count);
	}

	for (int i = 0; i < vertex_count; i++) {
		if(is_ascii)
			this->_read_ascii_vertices(file,values,taille);
		else
			this->_read_binary_vertices(file,property_type,values,taille);
		vertices[i] = Vector3(values[idxs[0]], values[idxs[1]], values[idxs[2]]);
		if (has_normals)
			normals[i] = Vector3(values[idxs[3]], values[idxs[4]], values[idxs[5]]);
		if (has_colors) {
			if (has_alpha)
				colors[i] = Color(values[idxs[6]]/255, values[idxs[7]]/255, values[idxs[8]]/255, values[idxs[9]]/255);
			else
				colors[i] = Color(values[idxs[6]]/255, values[idxs[7]]/255, values[idxs[8]]/255);
		}
		else
			colors[i] = Color(0.85,0.85,0.85);
		if (has_uvs)
			uv_mapping[i] = Vector2(values[idxs[10]], values[idxs[11]]);
	}
	delete[] values;
	values = nullptr;
	if(is_ascii)
		this->_read_ascii_faces(file,faces,face_count);
	else
		this->_read_binary_faces(file,faces,face_types,face_count);
	return this->data_to_arrays(vertices, this->face_to_indices(faces), colors, normals, uv_mapping);
}

ArrayMesh* PlyReader::create_mesh(const Array &arrays, bool pointCloud)const {
	// Création de l'ArrayMesh et ajout de la surface depuis les tableaux.
	ArrayMesh *mesh = memnew(ArrayMesh);
	if (pointCloud) {
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_POINTS, arrays);
	} else {
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	}

	// Création d'un matériel qui utilise les couleurs par sommet.
	StandardMaterial3D* mat = memnew(StandardMaterial3D);
	mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR,true);
	mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX);
	mat->set_diffuse_mode(BaseMaterial3D::DIFFUSE_LAMBERT);
	mesh->surface_set_material(0, mat);

	return mesh;
}

ArrayMesh* PlyReader::load_ply(const String& file_path)const {
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
	PackedInt32Array property_type;
	int face_type[2];

	line = file->get_line().strip_edges();
	while (line != "end_header") {
		if (line.begins_with("element vertex")) {
			vertex_count = line.get_slice(" ", 2).to_int();
		} else if (line.begins_with("element face")) {
			face_count = line.get_slice(" ", 2).to_int();
			// Pour la face, on lit une ligne supplémentaire pour obtenir le type.
			line = file->get_line().strip_edges();
			face_type[0]=(this->_type_to_bits(line.get_slice(" ", 2)));
			face_type[1]=(this->_type_to_bits(line.get_slice(" ", 3)));
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
	
	Array arrays = this->_read_ply(file, property, property_type, face_type, vertex_count, face_count,is_ascii);
	if (arrays.size() != 13)
		return nullptr;
	file->close();
	return this->create_mesh(arrays, face_count == 0);
}

