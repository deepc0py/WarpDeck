// WarpDeck Website JavaScript

// Platform detection and download optimization
class PlatformDetector {
    constructor() {
        this.platform = this.detectPlatform();
        this.init();
    }

    detectPlatform() {
        const userAgent = navigator.userAgent.toLowerCase();
        const platform = navigator.platform.toLowerCase();

        if (platform.includes('mac') || userAgent.includes('macintosh')) {
            return 'macos';
        } else if (platform.includes('linux') || userAgent.includes('linux')) {
            // Check for Steam Deck specific indicators
            if (userAgent.includes('steamdeck') || 
                userAgent.includes('steamos') ||
                screen.width === 1280 && screen.height === 800) {
                return 'steamdeck';
            }
            return 'linux';
        } else if (platform.includes('win') || userAgent.includes('windows')) {
            return 'windows';
        }
        return 'unknown';
    }

    init() {
        this.optimizeDownloadLinks();
        this.showPlatformSpecificContent();
    }

    optimizeDownloadLinks() {
        const platformMap = {
            'macos': {
                primary: 'https://github.com/deepc0py/WarpDeck/releases/latest/download/WarpDeck-macOS.dmg',
                text: 'Download for macOS',
                format: 'DMG Installer',
                size: '25 MB'
            },
            'linux': {
                primary: 'https://github.com/deepc0py/WarpDeck/releases/latest/download/WarpDeck.AppImage',
                text: 'Download for Linux',
                format: 'AppImage',
                size: '45 MB'
            },
            'steamdeck': {
                primary: 'https://github.com/deepc0py/WarpDeck/releases/latest/download/WarpDeck.AppImage',
                text: 'Download for Steam Deck',
                format: 'AppImage (Steam Deck Optimized)',
                size: '45 MB'
            }
        };

        const platformInfo = platformMap[this.platform];
        if (platformInfo) {
            // Update hero download button
            const heroButton = document.querySelector('.hero-buttons .btn-primary');
            if (heroButton) {
                heroButton.href = platformInfo.primary;
                heroButton.innerHTML = `
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none">
                        <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" stroke="currentColor" stroke-width="2"/>
                        <polyline points="7,10 12,15 17,10" stroke="currentColor" stroke-width="2"/>
                        <line x1="12" y1="15" x2="12" y2="3" stroke="currentColor" stroke-width="2"/>
                    </svg>
                    ${platformInfo.text}
                `;
            }

            // Highlight the appropriate download card
            const downloadCards = document.querySelectorAll('.download-card');
            downloadCards.forEach(card => {
                const platformText = card.querySelector('h3').textContent.toLowerCase();
                if ((this.platform === 'macos' && platformText.includes('macos')) ||
                    (this.platform === 'linux' && platformText.includes('linux') && !platformText.includes('steam')) ||
                    (this.platform === 'steamdeck' && platformText.includes('steam'))) {
                    card.classList.add('download-featured');
                }
            });
        }
    }

    showPlatformSpecificContent() {
        // Add platform-specific messaging
        const platformMessages = {
            'macos': 'Optimized for Apple Silicon and Intel Macs',
            'linux': 'Universal compatibility across all Linux distributions',
            'steamdeck': 'Touch-optimized interface with gamepad support',
            'windows': 'Windows support coming soon!'
        };

        const message = platformMessages[this.platform];
        if (message && this.platform !== 'unknown') {
            const heroDesc = document.querySelector('.hero-description');
            if (heroDesc) {
                heroDesc.innerHTML += `<br><strong>${message}</strong>`;
            }
        }
    }
}

// Smooth scrolling for navigation links
class NavigationHandler {
    constructor() {
        this.init();
    }

    init() {
        // Smooth scroll for anchor links
        document.querySelectorAll('a[href^="#"]').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const target = document.querySelector(link.getAttribute('href'));
                if (target) {
                    target.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                }
            });
        });

        // Mobile navigation toggle
        const mobileToggle = document.querySelector('.nav-mobile-toggle');
        const navLinks = document.querySelector('.nav-links');
        
        if (mobileToggle && navLinks) {
            mobileToggle.addEventListener('click', () => {
                navLinks.classList.toggle('nav-mobile-open');
            });
        }

        // Active navigation highlighting
        this.highlightActiveSection();
    }

    highlightActiveSection() {
        const sections = document.querySelectorAll('section[id]');
        const navLinks = document.querySelectorAll('.nav-link[href^="#"]');

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const id = entry.target.getAttribute('id');
                    navLinks.forEach(link => {
                        link.classList.remove('nav-active');
                        if (link.getAttribute('href') === `#${id}`) {
                            link.classList.add('nav-active');
                        }
                    });
                }
            });
        }, { threshold: 0.3 });

        sections.forEach(section => observer.observe(section));
    }
}

// Analytics and interaction tracking
class AnalyticsTracker {
    constructor() {
        this.events = [];
        this.init();
    }

    init() {
        this.trackDownloads();
        this.trackExternalLinks();
        this.trackScrollDepth();
    }

    trackDownloads() {
        document.querySelectorAll('.download-btn, .btn-primary[href*="download"], .btn-primary[href*="releases"]').forEach(button => {
            button.addEventListener('click', (e) => {
                const platform = this.extractPlatformFromUrl(button.href) || 'unknown';
                this.trackEvent('download', 'click', platform);
            });
        });
    }

    trackExternalLinks() {
        document.querySelectorAll('a[href^="http"]').forEach(link => {
            if (!link.href.includes(window.location.hostname)) {
                link.addEventListener('click', (e) => {
                    const destination = new URL(link.href).hostname;
                    this.trackEvent('external_link', 'click', destination);
                });
            }
        });
    }

    trackScrollDepth() {
        let maxScrollDepth = 0;
        const trackingPoints = [25, 50, 75, 90, 100];

        window.addEventListener('scroll', () => {
            const scrollDepth = Math.round(
                (window.scrollY + window.innerHeight) / document.body.scrollHeight * 100
            );

            if (scrollDepth > maxScrollDepth) {
                maxScrollDepth = scrollDepth;
                
                trackingPoints.forEach(point => {
                    if (scrollDepth >= point && !this.events.some(e => 
                        e.action === 'scroll_depth' && e.label === `${point}%`)) {
                        this.trackEvent('engagement', 'scroll_depth', `${point}%`);
                    }
                });
            }
        });
    }

    extractPlatformFromUrl(url) {
        if (url.includes('macOS')) return 'macos';
        if (url.includes('AppImage')) return 'linux';
        if (url.includes('Steam')) return 'steamdeck';
        return null;
    }

    trackEvent(category, action, label) {
        const event = {
            category,
            action,
            label,
            timestamp: Date.now(),
            url: window.location.href,
            userAgent: navigator.userAgent
        };

        this.events.push(event);
        
        // Send to analytics service (privacy-respecting)
        // Note: In production, this would send to your analytics endpoint
        console.log('Analytics Event:', event);
    }
}

// Performance monitoring
class PerformanceMonitor {
    constructor() {
        this.metrics = {};
        this.init();
    }

    init() {
        // Track page load performance
        window.addEventListener('load', () => {
            setTimeout(() => {
                this.collectMetrics();
            }, 100);
        });
    }

    collectMetrics() {
        if ('performance' in window) {
            const perfData = performance.getEntriesByType('navigation')[0];
            
            this.metrics = {
                loadTime: perfData.loadEventEnd - perfData.loadEventStart,
                domContentLoaded: perfData.domContentLoadedEventEnd - perfData.domContentLoadedEventStart,
                firstContentfulPaint: this.getFCP(),
                largestContentfulPaint: this.getLCP()
            };

            // Send performance data (in production)
            console.log('Performance Metrics:', this.metrics);
        }
    }

    getFCP() {
        const fcpEntry = performance.getEntriesByName('first-contentful-paint')[0];
        return fcpEntry ? fcpEntry.startTime : null;
    }

    getLCP() {
        return new Promise((resolve) => {
            new PerformanceObserver((list) => {
                const entries = list.getEntries();
                const lastEntry = entries[entries.length - 1];
                resolve(lastEntry.startTime);
            }).observe({ entryTypes: ['largest-contentful-paint'] });
        });
    }
}

// Easter eggs and fun interactions
class EasterEggs {
    constructor() {
        this.init();
    }

    init() {
        this.konami();
        this.clickCounter();
        this.steamDeckAnimation();
    }

    konami() {
        const sequence = ['ArrowUp', 'ArrowUp', 'ArrowDown', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'ArrowLeft', 'ArrowRight', 'KeyB', 'KeyA'];
        let index = 0;

        document.addEventListener('keydown', (e) => {
            if (e.code === sequence[index]) {
                index++;
                if (index === sequence.length) {
                    this.activateWarpSpeed();
                    index = 0;
                }
            } else {
                index = 0;
            }
        });
    }

    activateWarpSpeed() {
        document.body.style.animation = 'warp-speed 2s ease-in-out';
        
        const message = document.createElement('div');
        message.textContent = '🚀 WARP SPEED ACTIVATED! 🚀';
        message.style.cssText = `
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: linear-gradient(135deg, #2196F3, #1976D2);
            color: white;
            padding: 2rem;
            border-radius: 12px;
            font-size: 1.5rem;
            font-weight: bold;
            z-index: 10000;
            animation: warp-message 3s ease-in-out;
        `;
        
        document.body.appendChild(message);
        
        setTimeout(() => {
            message.remove();
            document.body.style.animation = '';
        }, 3000);
    }

    clickCounter() {
        let clicks = 0;
        const logo = document.querySelector('.nav-logo');
        
        if (logo) {
            logo.addEventListener('click', (e) => {
                e.preventDefault();
                clicks++;
                
                if (clicks === 10) {
                    logo.style.animation = 'spin 1s ease-in-out infinite';
                    setTimeout(() => {
                        logo.style.animation = '';
                        clicks = 0;
                    }, 3000);
                }
            });
        }
    }

    steamDeckAnimation() {
        const steamDeckCard = document.querySelector('.download-card:has(h3:contains("Steam Deck"))');
        if (steamDeckCard) {
            steamDeckCard.addEventListener('mouseenter', () => {
                const icon = steamDeckCard.querySelector('.platform-icon');
                if (icon) {
                    icon.style.animation = 'steam-deck-glow 0.5s ease-in-out';
                }
            });
        }
    }
}

// Initialize everything when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new PlatformDetector();
    new NavigationHandler();
    new AnalyticsTracker();
    new PerformanceMonitor();
    new EasterEggs();
});

// Add CSS animations
const style = document.createElement('style');
style.textContent = `
    @keyframes warp-speed {
        0% { transform: scale(1); }
        50% { transform: scale(1.05) rotate(1deg); }
        100% { transform: scale(1); }
    }
    
    @keyframes warp-message {
        0% { opacity: 0; transform: translate(-50%, -50%) scale(0.5); }
        20% { opacity: 1; transform: translate(-50%, -50%) scale(1.1); }
        80% { opacity: 1; transform: translate(-50%, -50%) scale(1); }
        100% { opacity: 0; transform: translate(-50%, -50%) scale(0.8); }
    }
    
    @keyframes spin {
        from { transform: rotate(0deg); }
        to { transform: rotate(360deg); }
    }
    
    @keyframes steam-deck-glow {
        0% { text-shadow: none; }
        50% { text-shadow: 0 0 20px #2196F3; }
        100% { text-shadow: none; }
    }
    
    .nav-active {
        color: var(--primary) !important;
    }
    
    .nav-mobile-open {
        display: flex !important;
        flex-direction: column;
        position: absolute;
        top: 100%;
        left: 0;
        right: 0;
        background: white;
        padding: 1rem 2rem;
        border-bottom: 1px solid var(--border);
        box-shadow: 0 4px 24px var(--shadow);
    }
`;
document.head.appendChild(style);