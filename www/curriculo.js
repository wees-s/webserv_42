let active = '';

// Auto-detect active template from URL or active class
function detectActiveTemplate() {
  // Try to get from URL first
  const path = window.location.pathname;
  if (path.includes('moderno.html')) return 'moderno';
  if (path.includes('profissional.html')) return 'profissional';
  if (path.includes('academico.html')) return 'academico';
  if (path.includes('criativo.html')) return 'criativo';
  if (path.includes('classico.html')) return 'classico';
  
  // Fallback: check for active cv-sheet
  const activeSheet = document.querySelector('.cv-sheet.active');
  if (activeSheet) {
    const id = activeSheet.id;
    if (id) return id.replace('cv-', '');
  }
  
  return 'classico'; // Default fallback
}

// Initialize active template
active = detectActiveTemplate();

// Load curriculum data from server on page load
async function loadCurriculum() {
  try {
    const response = await fetch('/api/curriculum');
    const data = await response.json();
    
    if (Object.keys(data).length > 0) {
      // Populate fields from loaded data
      Object.entries(FM).forEach(([id, field]) => {
        if (data[field]) {
          const el = document.getElementById(id);
          if (el) el.value = data[field];
          setAll(field, data[field]);
        }
      });
      
      // Load photo if exists
      if (data.photoUrl) {
        cvData.photoUrl = data.photoUrl;
        const photoEl = document.getElementById('cv-' + active).querySelector('[data-photo]');
        if (photoEl) {
          photoEl.style.backgroundImage = `url(${data.photoUrl})`;
        }
        
        // Show photo link
        const photoLink = document.getElementById('photo-link');
        if (photoLink) {
          photoLink.href = data.photoUrl;
          photoLink.style.display = 'inline-block';
        }
        
        // Set photoUrl in form
        const photoFormField = document.getElementById('form-photoUrl');
        if (photoFormField) photoFormField.value = data.photoUrl;
      }
    }
  } catch (error) {
    console.error('Error loading curriculum:', error);
  }
}

// Load curriculum data when page loads
document.addEventListener('DOMContentLoaded', loadCurriculum);

// Data storage
let cvData = {
  name: '', cargo: '', email: '', phone: '', city: '', sobre: '',
  photoUrl: '',
  experiences: [],
  education: [],
  skills: []
};

// Field map for all fields
const FM = {
  'f-name':'name','f-cargo':'cargo','f-email':'email','f-phone':'phone','f-city':'city','f-sobre':'sobre',
  'f-skill1':'skill1','f-skill2':'skill2','f-skill3':'skill3','f-skill4':'skill4','f-skill5':'skill5','f-skill6':'skill6',
  'f-expcargo1':'expcargo1','f-expempresa1':'expempresa1','f-expdesc1':'expdesc1',
  'f-expcargo2':'expcargo2','f-expempresa2':'expempresa2','f-expdesc2':'expdesc2',
  'f-expcargo3':'expcargo3','f-expempresa3':'expempresa3','f-expdesc3':'expdesc3',
  'f-educurso1':'educurso1','f-eduinst1':'eduinst1',
  'f-educurso2':'educurso2','f-eduinst2':'eduinst2',
  'f-educurso3':'educurso3','f-eduinst3':'eduinst3'
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

function clearAllData() {
  if (!confirm('Tem certeza que deseja limpar todos os dados?')) return;
  
  fetch('/api/curriculum', { method: 'DELETE' })
    .then(() => {
      // Clear all fields in the form
      Object.entries(FM).forEach(([id, f]) => {
        const input = document.getElementById(id);
        if (input) input.value = '';
        setAll(f, '');
      });
      
      // Clear photo
      cvData.photoUrl = '';
      const sheet = document.getElementById('cv-' + active);
      if (sheet) {
        const photoEl = sheet.querySelector('.cl-photo, .pr-photo');
        if (photoEl) {
          photoEl.style.backgroundImage = '';
          photoEl.textContent = '👤';
        }
      }
      
      // Clear photo link
      const photoLink = document.getElementById('photo-link');
      if (photoLink) {
        photoLink.style.display = 'none';
      }
      
      // Clear form photoUrl
      const photoFormField = document.getElementById('form-photoUrl');
      if (photoFormField) photoFormField.value = '';
      
      showToast('Dados limpos com sucesso!');
    })
    .catch(err => {
      console.error('Error clearing data:', err);
      showToast('Erro ao limpar dados');
    });
}

// Toggle field visibility in curriculum
function toggleField(field, btn) {
  const sheet = document.getElementById('cv-' + active);
  if (sheet) {
    const el = sheet.querySelector('[data-f="' + field + '"]');
    if (el) {
      el.classList.toggle('hidden');
      btn.classList.toggle('active');
    }
  }
}

// Photo upload
function handlePhotoUpload(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = function(e) {
    const photoUrl = e.target.result;
    cvData.photoUrl = photoUrl;
    
    const sheet = document.getElementById('cv-' + active);
    const photoEl = sheet.querySelector('.cl-photo, .pr-photo');
    if (photoEl) {
      photoEl.style.backgroundImage = `url(${photoUrl})`;
      photoEl.style.backgroundSize = 'cover';
      photoEl.style.backgroundPosition = 'center';
      photoEl.textContent = '';
    }
    const linkEl = document.getElementById('photo-link');
    if (linkEl) {
      linkEl.href = photoUrl;
      linkEl.download = 'foto-perfil.png';
      linkEl.style.display = 'inline-block';
    }
    
    // Set photoUrl in form
    const photoFormField = document.getElementById('form-photoUrl');
    if (photoFormField) photoFormField.value = photoUrl;
  };
  reader.readAsDataURL(file);
}

// PDF download
function downloadPDF() {
  showToast('Gerando PDF...');
  const sheet = document.getElementById('cv-' + active);
  if (!sheet) {
    showToast('Erro ao gerar PDF');
    return;
  }

  const opt = {
    margin: 0,
    filename: 'curriculo.pdf',
    image: { type: 'jpeg', quality: 0.98 },
    html2canvas: { scale: 2 },
    jsPDF: { unit: 'mm', format: 'a4', orientation: 'portrait' }
  };

  html2pdf().set(opt).from(sheet).save().then(() => {
    showToast('PDF gerado com sucesso!');
  }).catch(err => {
    console.error(err);
    const t = document.getElementById('toast');
    t.textContent = 'Erro ao gerar PDF'; t.classList.add('show');
    setTimeout(() => t.classList.remove('show'), 2400);
  });
}

function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg; t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2400);
}
