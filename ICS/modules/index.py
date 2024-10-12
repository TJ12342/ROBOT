from flask import Blueprint, jsonify, request, render_template

index_bp = Blueprint('index', __name__)

@index_bp.route('/', methods=['GET'])
def welcome():
  return render_template('welcome.html')


@index_bp.route('/index', methods=['GET'])
def index():
  return render_template('index.html')


@index_bp.route('/record', methods=['GET'])
def record():
  return render_template('record.html')