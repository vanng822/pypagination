# -*- coding: utf8 -*-

import unittest
import pagination

class PaginationTranslationTest(unittest.TestCase):
    def setUp(self):
        self.translations = {
            pagination.LC_NEXT : "Volgende",
            pagination.LC_PREVIOUS : "Voorgaand",
            pagination.LC_FIRST : "Eerst",
            pagination.LC_LAST : "Laatste",
            pagination.LC_CURRENT_PAGE_REPORT : "Resulten %d - %d van %d"
        }
    
    def test_render_search(self):
        p = pagination.Paginator(100,
                                prelink= '/',
                                current = 5,
                                pageLinks = 5,
                                translations = self.translations
                                )
        
        self.assertEqual(p.render(), '<div class="paginator"><a href="/?page=4" class="paginator-previous">Voorgaand</a><a href="/?page=3" class="paginator-page paginator-page-first">3</a><a href="/?page=4" class="paginator-page">4</a><a href="/?page=5" class="paginator-current">5</a><a href="/?page=6" class="paginator-page">6</a><a href="/?page=7" class="paginator-page paginator-page-last">7</a><a href="/?page=6" class="paginator-next">Volgende</a></div>')
            
    def test_render_item(self):
        p = pagination.Paginator(100,
                                prelink= '/',
                                pageLinks = 5,
                                current = 5,
                                translations = self.translations)
        
        self.assertEqual(p.render(pattern='item'), '<div class="paginator"><span class="paginator-current-report">Resulten 41 - 50 van 100</span><a href="/?page=1" class="paginator-first">Eerst</a><a href="/?page=4" class="paginator-previous">Voorgaand</a><a href="/?page=6" class="paginator-next">Volgende</a><a href="/?page=10" class="paginator-last">Laatste</a></div>')
     
     
        