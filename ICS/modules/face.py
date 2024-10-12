from flask import Blueprint, jsonify, request, render_template, session
from models.face import FaceFeature
from models.user import User
from db.db import db
import base64

face_bp = Blueprint('face', __name__)


@face_bp.route('/add', methods=['POST'])
def add():
  data = request.get_json()

  # 处理 features 字段
  features = data.get('features')
  if features is not None:
    features = base64.b64decode(features)  # 解码 Base64 字符串


  new_face = FaceFeature(
    name=data['name'],
    gender=data['gender'],
    department=data['department'],
    major=data['major'],
    class_=data['class_'],
    features=features
  )
  db.session.add(new_face)
  db.session.commit()
  return jsonify({'message': '添加成功'})


@face_bp.route('/delete/<int:id>', methods=['DELETE'])
def delete(id):
  face = FaceFeature.query.get(id)
  if face:
    db.session.delete(face)
    db.session.commit()
    return jsonify({'message': '删除成功'}), 200
  else:
    return jsonify({'message': '未找到该人脸'}), 400
  

@face_bp.route('/update/<int:id>', methods=['PUT'])
def update(id):
  data = request.get_json()
  feature = FaceFeature.query.get_or_404(id)
  feature.name = data.get('name', feature.name)
  feature.gender = data.get('gender', feature.gender)
  feature.department = data.get('department', feature.department)
  feature.major = data.get('major', feature.major)
  feature.class_ = data.get('class_', feature.class_)
  # 处理 features 字段
  if 'features' in data:
    if data['features'] is not None:  # 检查是否为 None
      feature.features = base64.b64decode(data['features'])  # 解码 Base64 字符串
    else:
      feature.features = None  # 如果为 None，则设置为 None
  db.session.commit()
  return jsonify({'message': '更新成功'}), 200

  
@face_bp.route('/list', methods=['GET'])
def list():

  query = FaceFeature.query
    
  features = query.all()
  results = [{
    'id': feature.id,
    'name': feature.name,
    'gender': feature.gender,
    'department': feature.department,
    'major': feature.major,
    'class_': feature.class_,
    'features': base64.b64encode(feature.features).decode('utf-8') if feature.features else None,  # 转换为Base64,
    'created_at': feature.created_at
  } for feature in features]

  return jsonify({"message": "查询成功", "data": results})
