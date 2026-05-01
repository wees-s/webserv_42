function createStars() {
    const starsContainer = document.getElementById('stars');
    const numberOfStars = 200;
    
    for (let i = 0; i < numberOfStars; i++) {
        const star = document.createElement('div');
        star.className = 'star';
        
        const size = Math.random() * 3 + 1;
        star.style.width = size + 'px';
        star.style.height = size + 'px';
        star.style.left = Math.random() * 100 + '%';
        star.style.top = Math.random() * 100 + '%';
        star.style.animationDelay = Math.random() * 2 + 's';
        star.style.animationDuration = (Math.random() * 2 + 1) + 's';
        
        starsContainer.appendChild(star);
    }
}

document.addEventListener('DOMContentLoaded', function() {
    createStars();
    
    const form = document.querySelector('form');
    if (form) {
        const planetaSelect = document.getElementById('planeta');
        const curiosidadeTextarea = document.getElementById('curiosidade');
        
        if (planetaSelect && curiosidadeTextarea) {
            form.addEventListener('submit', function(e) {
                e.preventDefault();
                const planeta = planetaSelect.value;
                const curiosidade = curiosidadeTextarea.value;
                
                if (planeta && curiosidade) {
                    saveCuriosidade(planeta, curiosidade);
                    alert('Curiosidade enviada com sucesso!');
                    form.reset();
                } else {
                    alert('Por favor, preencha todos os campos.');
                }
            });
        }
    }
    
    loadCuriosidades();
});

function saveCuriosidade(planeta, curiosidade) {
    const curiosidades = JSON.parse(localStorage.getItem('curiosidades')) || {};
    if (!curiosidades[planeta]) {
        curiosidades[planeta] = [];
    }
    curiosidades[planeta].push({
        texto: curiosidade,
        data: new Date().toLocaleString('pt-BR'),
        likes: 0
    });
    localStorage.setItem('curiosidades', JSON.stringify(curiosidades));
}

function loadCuriosidades() {
    const container = document.getElementById('curiosidades-container');
    if (!container) return;
    
    const currentPage = window.location.pathname.replace(/\\/g, '/').split('/').pop().replace('.html', '');
    const curiosidades = JSON.parse(localStorage.getItem('curiosidades')) || {};
    let planetCuriosidades = curiosidades[currentPage] || [];
    
    // Ensure all curiosidades have likes field (for backward compatibility)
    planetCuriosidades = planetCuriosidades.map(c => ({
        ...c,
        likes: c.likes || 0
    }));
    
    // Sort by likes (descending - most liked first)
    planetCuriosidades.sort((a, b) => b.likes - a.likes);
    
    if (planetCuriosidades.length === 0) {
        container.innerHTML = '<p style="text-align: center; color: rgba(255,255,255,0.5);">Nenhuma curiosidade adicionada ainda.</p>';
        return;
    }
    
    container.innerHTML = '';
    planetCuriosidades.forEach((curiosidade, index) => {
        const box = document.createElement('div');
        box.className = 'curiosidade-box';
        box.innerHTML = `
            <p>${curiosidade.texto}</p>
            <small style="color: rgba(255,255,255,0.5);">${curiosidade.data}</small>
            <div class="actions">
                <button class="like-btn" onclick="likeCuriosidade('${currentPage}', ${index})">❤️ ${curiosidade.likes}</button>
                <button class="delete-btn" onclick="deleteCuriosidade('${currentPage}', ${index})">Excluir</button>
            </div>
        `;
        container.appendChild(box);
    });
}

function deleteCuriosidade(planeta, index) {
    if (!confirm('Tem certeza que deseja excluir esta curiosidade?')) return;
    
    const curiosidades = JSON.parse(localStorage.getItem('curiosidades')) || {};
    if (curiosidades[planeta] && curiosidades[planeta][index]) {
        curiosidades[planeta].splice(index, 1);
        localStorage.setItem('curiosidades', JSON.stringify(curiosidades));
        loadCuriosidades();
    }
}

function likeCuriosidade(planeta, index) {
    const curiosidades = JSON.parse(localStorage.getItem('curiosidades')) || {};
    if (curiosidades[planeta] && curiosidades[planeta][index]) {
        curiosidades[planeta][index].likes = (curiosidades[planeta][index].likes || 0) + 1;
        localStorage.setItem('curiosidades', JSON.stringify(curiosidades));
        loadCuriosidades();
    }
}
