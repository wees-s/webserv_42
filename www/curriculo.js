let active = 'classico';

function goTo(id) {
  document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
  document.getElementById(id).classList.remove('hidden');
}

function selectTemplate(tpl) {
  active = tpl;
  document.querySelectorAll('.cv-sheet').forEach(s => s.classList.remove('active'));
  document.getElementById('cv-' + tpl).classList.add('active');
  goTo('screen-editor');
}

// field map: input-id → data-f value
const FM = {
  'f-name':'name','f-cargo':'cargo','f-email':'email','f-phone':'phone','f-city':'city',
  'f-sobre':'sobre','f-skill1':'skill1','f-skill2':'skill2',
  'f-expcargo':'expcargo','f-expempresa':'expempresa','f-expdesc':'expdesc',
  'f-educurso':'educurso','f-eduinst':'eduinst'
};

function getVal(field) {
  const sheet = document.getElementById('cv-' + active);
  const el = sheet && sheet.querySelector('[data-f="' + field + '"]');
  return el ? el.textContent : '';
}

function setAll(field, val) {
  if (!val) return;
  document.querySelectorAll('[data-f="' + field + '"]').forEach(el => el.textContent = val);
}

function openModal() {
  Object.entries(FM).forEach(([id, f]) => { document.getElementById(id).value = getVal(f); });
  document.getElementById('modal-overlay').classList.add('open');
}

function closeModal() { document.getElementById('modal-overlay').classList.remove('open'); }

function handleOverlay(e) { if (e.target === document.getElementById('modal-overlay')) closeModal(); }

function saveModal() {
  Object.entries(FM).forEach(([id, f]) => setAll(f, document.getElementById(id).value.trim()));
  closeModal();
  showToast('Currículo atualizado!');
}

function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg; t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2400);
}

