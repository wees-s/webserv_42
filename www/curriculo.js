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
let active = detectActiveTemplate();

// Load last updated date from CGI
function loadLastUpdated() {
  const lastUpdatedEl = document.getElementById('last-updated');
  if (lastUpdatedEl) {
    // Call CGI directly (no longer needs PID)
    fetch('/cgi-bin/cgiGet.py')
      .then(response => response.json())
      .then(data => {
        lastUpdatedEl.textContent = 'Ultima edição: ' + data.datetime;
      })
      .catch(error => {
        console.error('Error loading last updated:', error);
      });
  }
}

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
        const cvSheet = document.getElementById('cv-' + active);
        if (cvSheet) {
          const photoEl = cvSheet.querySelector('[data-photo]');
          if (photoEl) {
            photoEl.style.backgroundImage = `url(${data.photoUrl})`;
            photoEl.textContent = ''; // Remove user icon when image is loaded
            photoEl.style.cursor = 'pointer';
            photoEl.onclick = () => {
              window.open(data.photoUrl, '_blank');
            };
          }
        }
      } else {
        // No photo, reset to default
        cvData.photoUrl = '';
        const cvSheet = document.getElementById('cv-' + active);
        if (cvSheet) {
          const photoEl = cvSheet.querySelector('[data-photo]');
          if (photoEl) {
            photoEl.style.backgroundImage = '';
            photoEl.textContent = '👤';
            photoEl.style.cursor = 'default';
            photoEl.onclick = null;
          }
        }
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
  photoUrl: ''
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
  Object.entries(FM).forEach(([id, f]) => {
    const el = document.getElementById(id);
    const val = getVal(f);
    el.placeholder = val;
    el.value = '';
    // Ao focar, limpa o placeholder se o campo estiver vazio
    el.onfocus = function() {
      if (this.value === '') {
        this.placeholder = '';
      }
    };
    // Ao desfocar, se estiver vazio, restaura o placeholder
    el.onblur = function() {
      if (this.value === '') {
        this.placeholder = val;
      }
    };
  });
  document.getElementById('modal-overlay').classList.add('open');
}

function closeModal() { document.getElementById('modal-overlay').classList.remove('open'); }

function handleOverlay(e) { if (e.target === document.getElementById('modal-overlay')) closeModal(); }

// Handle form submit - prevent default behavior and just close modal
document.addEventListener('DOMContentLoaded', function() {
  const form = document.getElementById('curriculum-form');
  if (form) {
    form.addEventListener('submit', function(e) {
      closeModal();
    });
  }
});

function clearAllData() {
  if (!confirm('Tem certeza que deseja limpar todos os dados?')) return;
  
  fetch('/api/curriculum', { method: 'DELETE' })
    .then(response => {
      if (!response.ok)
        throw new Error('Erro no servidor');

      // Recarrega a página para mostrar valores padrão do HTML
      location.reload();
      showToast('Dados limpos com sucesso!');
    })
    .catch(() => {
      showToast('Erro ao limpar dados');
    });
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

// Load last updated date if on templates page
if (window.location.pathname.includes('templates.html')) {
  loadLastUpdated();
}