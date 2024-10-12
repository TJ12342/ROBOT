from flask import Blueprint, jsonify, request, render_template, session
from models.user import User
from db.db import db

user_bp = Blueprint('user', __name__)

@user_bp.route('/login', methods=['GET', 'POST'])
def login():
  if(request.method == 'GET'):
    return render_template('login.html')
  
  data = request.get_json()
  username = data['username']
  password = data['password']

  if not username or not password:
    return jsonify({'message': '用户名和密码不能为空'}), 400
  
  exist_user = User.query.filter_by(username=username).first()

  if not exist_user:
    return jsonify({'message': '用户不存在'}), 400
  
  if exist_user.password != password:
    return jsonify({'message': '密码错误'}), 400
  
  session['username'] = username
  session['login_in'] = True

  user_data = {
    'id': exist_user.id,
    'username': exist_user.username
  }

  return jsonify({'message': '登录成功', 'data': user_data}), 200


@user_bp.route('/register', methods=['GET', 'POST'])
def register():

  if(request.method == 'GET'):
    return render_template('register.html')

  data = request.get_json()
  username = data['username']
  password = data['password']

  if not username or not password:
    return jsonify({'message': '用户名和密码不能为空'}), 400
  
  exist_user = User.query.filter_by(username=username).first()
  if exist_user:
    return jsonify({'message': '用户已存在'}), 400
  
  create_user = User(username=username, password=password)
  db.session.add(create_user)
  db.session.commit()

  return jsonify({'message': '注册成功'})


@user_bp.route('/logout', methods=['GET'])
def logout():
  session.pop('username', None)
  session.pop('login_in', None)
  return jsonify({'message': '退出登录成功'}), 200


@user_bp.route('/list', methods=['GET'])
def list():
  users = User.query.all()
  user_list = [{'id': user.id, 'username': user.username} for user in users]
  return jsonify({'message': '查询成功', 'data': user_list}), 200