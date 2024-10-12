

function toLoginAndIndex() {
  let user = localStorage.getItem('user_data')
  if (user) {
    window.location.href = '/index'
  } else {
    window.location.href = '/user/login'
  }
}